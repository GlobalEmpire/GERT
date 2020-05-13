#include "NetString.h"
#include <cstring>

NetString::NetString(char len, char* str) : len(len), str(str) {}

NetString::~NetString() {
	delete this->str;
}

NetString NetString::extract(Connection* conn) {
	char * rawlen = conn->read(1);
	char * data = conn->read(rawlen[1]);

	if (data[0] < rawlen[1]) {
		rawlen[1] = data[0];
	}

	char len = rawlen[1]
	char* str = new char[len]

	memcpy(str, data+1, len[1]);
	delete len;
	delete data;

	return {
		len,
		str
	};
}

std::string NetString::string() const {
	return std::string{this->length} + std::string{this->str, (size_t)this->length};
}
