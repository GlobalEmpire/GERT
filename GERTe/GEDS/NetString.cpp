#include "NetString.h"
#include <cstring>

NetString::NetString(Connection * conn, int offset) {
	length = conn->bufsize;
	str = new char[length];
	memcpy(str, conn->buf, length);
}

NetString::~NetString() {
	delete[] str;
}

std::string NetString::string() const {
	return std::string{this->length} + std::string{this->str, (size_t)this->length};
}
