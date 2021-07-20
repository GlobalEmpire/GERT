#include "Key.h"
#include "../Util/Crypto.h"
#include "../Util/logging.h"
#include <openssl/evp.h>
#include <utility>
#include <fstream>

#ifdef WIN32
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

Key::Key(std::string keyin) : key(std::move(keyin)) {}

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

bool Key::loadKeys(const std::string& file) {
    std::fstream fstr{ file };

    if (!fstr) { //If the file failed to open
        error("Failed to open key file");
        return false;
    }

    while (!fstr.eof()) {
        char bufE[3]; //Create a storage variable for the external portion of the address
        fstr.read(bufE, 3);

        if (!fstr)
            break;

        Address addr{std::string{ (char*)bufE, 3 }}; //Reformat the address portions into a single structure

        unsigned short length;
        fstr.read((char*)&length, 2);

        if (!fstr)
            break;

        length = ntohs(length);

        char* buf = new char[length];
        fstr.read(buf, length);

        if (!fstr)
            break;

        Key key({buf, length}); //Reformat the key
        delete[] buf;

        log("Imported resolution for " + addr.stringify()); //Print what we've imported
        Key::resolutions.insert({addr, key});
    }

    return true; //Return without an error
}
