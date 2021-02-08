#pragma once
#include <string>

std::string CryptoRandom(int);
void* CryptoNewKey();
std::string CryptoSharedSecret(void* pubKey, void* privKey);
void* CryptoReadKey(const std::string&);
std::string CryptoHMAC(std::string, std::string);

std::string CryptoExportPubKey(void*);
