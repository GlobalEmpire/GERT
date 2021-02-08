#include "Crypto.h"
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/x509.h>
#include <stdexcept>

[[noreturn]] void CryptoError() {
    char* err = new char[256];
    ERR_error_string_n(ERR_get_error(), err, 256);

    std::string errStr = err;
    delete[] err;
    throw std::runtime_error{ errStr };
}

EVP_PKEY* CryptoNewParams() {
    EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);

    if (pctx == nullptr)
        CryptoError();

    if(!EVP_PKEY_paramgen_init(pctx)
       || !EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1)) {
        EVP_PKEY_CTX_free(pctx);
        CryptoError();
    }

    EVP_PKEY* params = EVP_PKEY_new();

    if (params == nullptr) {
        EVP_PKEY_CTX_free(pctx);
        CryptoError();
    }

    if (!EVP_PKEY_paramgen(pctx, &params)) {
        EVP_PKEY_CTX_free(pctx);
        EVP_PKEY_free(params);
        CryptoError();
    }

    EVP_PKEY_CTX_free(pctx);
    return params;
}

std::string CryptoRandom(int length) {
    char* buf = new char[length];

    if (RAND_priv_bytes((unsigned char*)buf, length) != 1) {
        delete[] buf;
        CryptoError();
    }

    std::string output = buf;
    delete[] buf;
    return output;
}

void* CryptoNewKey() {
    EVP_PKEY* params = CryptoNewParams();
    EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new(params, nullptr);
    EVP_PKEY_free(params);

    if (kctx == nullptr) {
        EVP_PKEY_free(params);
        CryptoError();
    }

    if(!EVP_PKEY_keygen_init(kctx)) {
        EVP_PKEY_CTX_free(kctx);
        CryptoError();
    }

    EVP_PKEY* key = EVP_PKEY_new();

    if (key == nullptr) {
        EVP_PKEY_CTX_free(kctx);
        CryptoError();
    }

    if (!EVP_PKEY_keygen(kctx, &key)) {
        EVP_PKEY_CTX_free(kctx);
        EVP_PKEY_free(params);
        CryptoError();
    }

    EVP_PKEY_CTX_free(kctx);
    return key;
}

std::string CryptoSharedSecret(void* pubPtr, void* privPtr) {
    auto* pubKey = (EVP_PKEY*)pubPtr;
    auto* privKey = (EVP_PKEY*)privPtr;

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privKey, nullptr);
    if (ctx == nullptr)
        CryptoError();

    size_t secretLen;

    if (EVP_PKEY_derive_init(ctx) <= 0 || EVP_PKEY_derive_set_peer(ctx, pubKey) <= 0
            || EVP_PKEY_derive(ctx, nullptr, &secretLen) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        CryptoError();
    }

    auto* secret = new unsigned char[secretLen];

    if (EVP_PKEY_derive(ctx, secret, &secretLen) <= 0) {
        delete[] secret;
        EVP_PKEY_CTX_free(ctx);
        CryptoError();
    }
    std::string output{ (char*)secret, secretLen };
    delete[] secret;

    return output;
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

std::string CryptoHMAC(const std::string& data, const std::string& secret) {
    return nullptr;
}

std::string CryptoExportPubKey(void* ptr) {
    auto* pkey = (EVP_PKEY*)ptr;

    EC_KEY* key = EVP_PKEY_get0_EC_KEY(pkey);
    if (key == nullptr)
        CryptoError();

    unsigned char* buf = nullptr;
    unsigned long length = i2d_EC_PUBKEY(key, &buf);

    if (length <= 0)
        CryptoError();

    return std::string{ (char*)buf, length };
}
