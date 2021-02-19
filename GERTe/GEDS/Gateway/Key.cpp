#include "../Networking/Connection.h"
#include "Key.h"
#include "../Util/Crypto.h"
#include <openssl/evp.h>
#include <utility>

Key::Key(std::string  keyin) : key(std::move(keyin)) {}

bool Key::operator==(const Key &comp) const {
    void* us = CryptoReadKey(key);
    void* them = CryptoReadKey(comp.key);
    bool result = EVP_PKEY_cmp((EVP_PKEY*)us, (EVP_PKEY*)them) == 1;
    CryptoFreeKey(us);
    CryptoFreeKey(them);
    return result;
}

bool Key::exists(Address addr) {
    return resolutions.contains(addr);
}

Key Key::retrieve(Address addr) {
    return resolutions.at(addr);
}
