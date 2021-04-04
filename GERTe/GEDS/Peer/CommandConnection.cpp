#include "../Gateway/Key.h"
#include "../Gateway/DataPacket.h"
#include "../Util/Versioning.h"
#include "../Util/Crypto.h"
#include "../Util/logging.h"
#include "../Networking/Route.h"
#include "CommandConnection.h"
#include "RelayPacket.h"
#include "QueryPacket.h"
#include "QueryPPacket.h"
#include <stdexcept>
#include <chrono>
#include <mutex>
using namespace std::chrono_literals;

std::vector<CommandConnection*> conns;
std::vector<DataPacket> delayed;
std::mutex lock;

CommandConnection::CommandConnection(SOCKET s) : Connection(s, "Peer") {
    write(ThisVersion.tostring());
    lock.lock();
    conns.push_back(this);
    lock.unlock();
}

CommandConnection::~CommandConnection() noexcept {
    for (auto client: clients)
        Route::unregisterRoute(client, this);

    lock.lock();
    for (auto iter = conns.begin(); iter != conns.end(); iter++) {
        if (*iter == this) {
            conns.erase(iter);
            break;
        }
    }
    lock.unlock();
}

void CommandConnection::send(const DataPacket& packet) {
    write("\5" + packet.raw + packet.signature);
}

void CommandConnection::process() {
    if (curPacket == nullptr) {
        switch(read(1)[0]) {
            case 0x00: {
                curPacket = new QueryPacket();
                break;
            }
            case 0x01: {
                curPacket = new QueryPPacket();
                break;
            }
            case 0x02: {
                curPacket = new QueryPacket();
                curPacket->command = 0x02;
                break;
            }
            case 0x03: {
                curPacket = new RelayPacket();
                break;
            }
        }
    }

    int needed = curPacket->needed();
    if (curPacket->parse(read(needed))) {
        switch(curPacket->command) {
            case 0x00: {
                auto* casted = (QueryPacket*)curPacket;
                Route* route = Route::getRoute(casted->query);
                if (route != nullptr) {
                    std::string cmd = "\1" + casted->query.tostring();
                    cmd += route->major;
                    cmd += route->minor;
                    cmd += (char)Route::directRoute(casted->query);
                    write(cmd);
                } else
                    write("\2" + casted->query.tostring());

                delete casted;
                curPacket = nullptr;
                break;
            }
            case 0x01: {
                auto* casted = (QueryPPacket*)curPacket;

                if (casted->direct) {
                    Route::registerRoute(casted->query, Route{casted->major, casted->minor, this}, false);

                    lock.lock();
                    for (auto iter = queued.begin(); iter != queued.end(); iter++) {
                        if (iter->destination.external == casted->query) {
                            send(*iter);
                            auto temp = iter--;
                            queued.erase((iter--) + 1);
                        }
                    }
                    lock.unlock();
                }

                delete casted;
                curPacket = nullptr;
                break;
            }
            case 0x02: {
                auto *casted = (QueryPacket *) curPacket;
                Route::unregisterRoute(casted->query, this);

                lock.lock();
                for (auto iter = queued.begin(); iter != queued.end(); iter++)
                    if (iter->destination.external == casted->query)
                        queued.erase((iter--) + 1);
                lock.unlock();

                delete casted;
                curPacket = nullptr;
                break;
            }
            case 0x03: {
                auto* casted = (RelayPacket*)curPacket;

                Route* route = Route::getRoute(casted->destination.external);
                if (Key::exists(casted->source.external) && route != nullptr) {
                    void* key = CryptoReadKey(Key::retrieve(casted->source.external).key);


                    std::chrono::time_point now = std::chrono::system_clock::now();
                    std::chrono::system_clock::time_point timestamp{ std::chrono::seconds { casted->timestamp } };

                    if (now - timestamp < 1min && CryptoVerify(casted->raw, casted->signature, key)) {
                        auto temp = DataPacket();
                        temp.parse(casted->subraw + casted->signature);
                        route->connection->send(temp);
                    }
                    else
                        warn("Decrypt error from " + casted->source.external.stringify());
                } else
                    write("\2" + casted->source.external.tostring());

                delete casted;
                curPacket = nullptr;
                break;
            }
        }
    }
}

void CommandConnection::attempt(const DataPacket& packet) {
    std::string cmd = "\0" + packet.destination.external.tostring();

    lock.lock();
    for (auto conn: conns) {
        if (conn->valid) {
            conn->queued.push_back(packet);
            conn->write(cmd);
        }
    }
    lock.lock();
}
