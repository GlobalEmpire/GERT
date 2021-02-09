#include "SecureConnection.h"
#include "../Gateway/Key.h"
#include "../Util/Versioning.h"
#include "../Util/Crypto.h"
#include "../Util/logging.h"
#include <stdexcept>

SecureConnection::SecureConnection(SOCKET s, const std::string& t, Logic* proc) : Connection(s, t), processor(proc) {
    try {
        addr = Address::extract(this);
        void *connKey = CryptoNewKey();

        if (!Key::exists(addr)) {
            warn(addr.stringify() + " attempted to connect but no key was found");
            write("\0\0\1");
            close();
        } else {
            Key gateKey = Key::retrieve(addr);
            secret = CryptoSharedSecret(gateKey.key, connKey);

            write(ThisVersion.tostring() + CryptoExportPubKey(connKey));
        }
    } catch(std::runtime_error& e) {
        error("Handshake with " + addr.stringify() + " failed due to libcrypt");
        write("\0\0\2");
        close();
    }
}

void SecureConnection::send(const DataPacket& packet) {
    if (packet.destination.external != addr)
        warn("Tried to send packet to the wrong destination");
    else
        write(packet.raw + CryptoHMAC(packet.raw, secret));
}

void SecureConnection::process() {
    // TODO: Update when read from Connection gets fixed.
    // TODO: Improve reentrancy

    int needed = curPacket.needed();
    if (curPacket.parse(read(needed))) {
        if (curPacket.source.external != addr) {
            warn(addr.stringify() + " attempted to spoof another gateway: forcefully disconnecting");
            close();
            return;
        }

        if (curPacket.hmac == CryptoHMAC(curPacket.raw, secret))
            processor->process(curPacket);
        else
            warn("Decrypt error from " + addr.stringify());

        curPacket = DataPacket{};
    }
}
