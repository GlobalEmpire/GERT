#include "Key.h"
#include "DataConnection.h"
#include "../Util/Crypto.h"
#include "../Util/logging.h"
#include "../Networking/Route.h"
#include "../Peer/CommandConnection.h"
#include <stdexcept>
#include <chrono>
using namespace std::chrono_literals;

DataConnection::DataConnection(SOCKET s) : Connection(s, "Gateway") {}

DataConnection::~DataConnection() noexcept {
    CryptoFreeKey(key);
    Route::unregisterRoute(addr, this);
}

void DataConnection::send(const DataPacket& packet) {
    if (packet.destination.external != addr)
        warn("Tried to send packet to the wrong destination");
    else
        write(packet.raw + (char)packet.signature.length() + packet.signature);
}

void DataConnection::_process() {
    try {
        int needed = curPacket.needed();
        if (curPacket.parse(read(needed))) {
            if (curPacket.source.external != addr) {
                warn(addr.stringify() + " attempted to spoof another gateway: forcefully disconnecting");
                close();
                return;
            }

            auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
            auto timestamp = std::chrono::time_point_cast<std::chrono::seconds>(
                    std::chrono::time_point<std::chrono::system_clock>{ std::chrono::seconds{ curPacket.timestamp } }
                    );

            if (now - timestamp < 1min && CryptoVerify(curPacket.raw, curPacket.signature, key)) {
                Route* route = Route::getRoute(curPacket.destination.external);
                if (route != nullptr)
                    route->connection->send(curPacket);
                else {
                    CommandConnection::attempt(curPacket);
                }
            } else
                warn("Decrypt error from " + addr.stringify());

            curPacket = DataPacket{};
        }
    } catch(std::exception& e) {
        error("Failed to process gateway packet: " + std::string{ e.what() });
        close();
    }
}

bool DataConnection::handshake() {
    try {
        if (!atleast(3))
            return false;

        addr = Address{ read(3) };

        if (Key::exists(addr)) {
            key = CryptoReadKey(Key::retrieve(addr).key);
            Route::registerRoute(addr, Route{ vers[0], vers[1], this }, true);
            return true;
        } else {
            warn(addr.stringify() + " attempted to connect but no key was found");
            write(std::string{"\0\0\1", 3});
        }
    } catch(std::exception& e) {
        error("Failed to construct gateway");
    }

    close();
    return false;
}
