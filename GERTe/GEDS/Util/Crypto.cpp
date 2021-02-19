#include "Crypto.h"
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/x509.h>
#include <stdexcept>

[[noreturn]] void CryptoError() {
    char err[256];
    ERR_error_string_n(ERR_get_error(), err, 256);
    throw std::runtime_error{ err };
}

void* CryptoReadKey(const std::string& encoded) {
    auto* ptr = (const unsigned char*)encoded.c_str();

    EC_KEY* key = d2i_EC_PUBKEY(&key, &ptr, encoded.length());
    if (key == nullptr)
        CryptoError();

    EVP_PKEY* pkey = EVP_PKEY_new();
    if (pkey == nullptr) {
        EC_KEY_free(key);
        CryptoError();
    }

    if (!EVP_PKEY_assign_EC_KEY(pkey, key)) {
        EC_KEY_free(key);
        EVP_PKEY_free(pkey);
        CryptoError();
    }

    return pkey;
}

bool CryptoVerify(const std::string& data, const std::string& signature, void* ptr) {
    auto* key = (EVP_PKEY*)ptr;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == nullptr)
        CryptoError();

    if (EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, key) != 1) {
        EVP_MD_CTX_free(ctx);
        CryptoError();
    }

    int result = EVP_DigestVerify(ctx, (unsigned char*)signature.c_str(), signature.length(), (unsigned char*)data.c_str(), data.length());
    EVP_MD_CTX_free(ctx);
    if (result != 1 && result != 0) {
        CryptoError();
    }

    return result == 1;
}

void CryptoFreeKey(void* key) {
    EVP_PKEY_free((EVP_PKEY*)key);
}
