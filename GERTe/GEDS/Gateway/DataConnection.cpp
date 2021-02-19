#include "Key.h"
#include "../Util/Versioning.h"
#include "../Util/Crypto.h"
#include "../Util/logging.h"
#include "DataConnection.h"
#include "../Networking/Route.h"
#include <stdexcept>

DataConnection::DataConnection(SOCKET s) : Connection(s, "Gateway") {
    addr = Address::extract(this);

    if (Key::exists(addr)) {
        key = CryptoReadKey(Key::retrieve(addr).key);
        write(ThisVersion.tostring());
    } else {
        warn(addr.stringify() + " attempted to connect but no key was found");
        write("\0\0\1");
        close();
    }

    Route::registerRoute(addr, Route{ vers[0], vers[1], this }, true);
}

DataConnection::~DataConnection() noexcept {
    CryptoFreeKey(key);
    Route::unregisterRoute(addr, this);
}

void DataConnection::send(const DataPacket& packet) {
    if (packet.destination.external != addr)
        warn("Tried to send packet to the wrong destination");
    else
        write(packet.raw + packet.signature);
}

void DataConnection::process() {
    int needed = curPacket.needed();
    if (curPacket.parse(read(needed))) {
        if (curPacket.source.external != addr) {
            warn(addr.stringify() + " attempted to spoof another gateway: forcefully disconnecting");
            close();
            return;
        }

        if (CryptoVerify(curPacket.raw, curPacket.signature, key)) {
            Route* route = Route::getRoute(curPacket.destination.external);
            if (route != nullptr)
                route->connection->send(curPacket);
        }
        else
            warn("Decrypt error from " + addr.stringify());

        curPacket = DataPacket{};
    }
}
