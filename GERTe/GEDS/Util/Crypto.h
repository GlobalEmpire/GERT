#pragma once
#include <string>

void* CryptoReadKey(const std::string&);
bool CryptoVerify(const std::string&, const std::string&, void* pubKey);
void CryptoFreeKey(void*);
