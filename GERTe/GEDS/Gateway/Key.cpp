#include "../Networking/Connection.h"
#include "Key.h"
#include "../Util/Crypto.h"
#include <openssl/evp.h>
#include <map>

static std::map<Address, Key> resolutions;

Key::Key(const std::string& keyin) : key(CryptoReadKey(keyin)) {}

bool Key::operator==(const Key &comp) const {
    return EVP_PKEY_cmp((EVP_PKEY*)key, (EVP_PKEY*)comp.key) == 1;
}

void Key::add(Address addr, Key key) {
	resolutions[addr] = key;
}

void Key::remove(Address addr) {
	resolutions.erase(addr);
}

bool Key::exists(Address addr) {
    return resolutions.contains(addr);
}

Key Key::retrieve(Address addr) {
    return resolutions[addr]
}
