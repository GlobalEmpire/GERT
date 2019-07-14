#include "NetString.h"
#include <cstring>

NetString::NetString(Connection * conn, int offset) {
	length = (unsigned char)conn->buf[offset];
	str = new char[length];
	memcpy(str, conn->buf + offset + 1, length);
}

NetString::~NetString() {
	delete[] str;
}

std::string NetString::string() const {
	return std::string{(char)length} + std::string{this->str, (size_t)this->length};
}
