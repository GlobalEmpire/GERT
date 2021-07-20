#include "../Gateway/Key.h"
#include "../Gateway/DataPacket.h"
#include "../Util/Crypto.h"
#include "../Util/logging.h"
#include "../Util/Error.h"
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
std::mutex lock;

CommandConnection::CommandConnection(uint32_t ip, uint16_t port) : Connection(ip, port, "Peer") {
    lock.lock();
    conns.push_back(this);
    lock.unlock();
}

CommandConnection::CommandConnection(ipv6 ip, uint16_t port) : Connection(ip, port, "Peer") {
    lock.lock();
    conns.push_back(this);
    lock.unlock();
}

CommandConnection::CommandConnection(SOCKET s) : Connection(s, "Peer") {
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
    write("\3" + packet.raw + (char)packet.signature.length() + packet.signature);
}

void CommandConnection::_process() {
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
            default: {
                error("Unknown command from peer.");
                close();
                return;
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
                    cmd += (char)route->major;
                    cmd += (char)route->minor;
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
                    Route::registerRoute(casted->query, Route{ casted->major, casted->minor, this }, false);

                    lock.lock();
                    std::erase_if(queued, [this, &casted](DataPacket& packet){
                        if (packet.destination.external == casted->query) {
                            send(packet);
                            return true;
                        } else
                            return false;
                    });
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
                std::erase_if(queued,
                              [&casted](DataPacket& packet){ return packet.destination.external == casted->query; });
                lock.unlock();

                delete casted;
                curPacket = nullptr;
                break;
            }
            case 0x03: {
                auto* casted = (RelayPacket*)curPacket;
                auto& dpacket = casted->subpacket;

                Route* route = Route::getRoute(dpacket.destination.external);
                if (Key::exists(dpacket.source.external) && route != nullptr) {
                    void* key = CryptoReadKey(Key::retrieve(dpacket.source.external).key);

                    std::chrono::time_point now = std::chrono::system_clock::now();
                    std::chrono::system_clock::time_point timestamp{ std::chrono::seconds { dpacket.timestamp } };

                    if (now - timestamp < 1min && CryptoVerify(dpacket.raw, dpacket.signature, key))
                        route->connection->send(dpacket);
                    else
                        warn("Decrypt error from " + dpacket.source.external.stringify());
                } else
                    write("\2" + dpacket.source.external.tostring());

                delete casted;
                curPacket = nullptr;
                break;
            }
        }
    }
}

void CommandConnection::attempt(const DataPacket& packet) {
    std::string cmd{ "\0", 1 };
    cmd += packet.destination.external.tostring();

    lock.lock();
    for (auto conn: conns) {
        if (conn->valid()) {
            conn->queued.push_back(packet);
            conn->write(cmd);
        }
    }
    lock.unlock();
}
