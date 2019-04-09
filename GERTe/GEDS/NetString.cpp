#include "NetString.h"
#include <cstring>

NetString::NetString(Connection * conn) {
	char * len = conn->read(1);
	char * data = conn->read(len[1]);

	if (data[0] < len[1]) {
		len[1] = data[0];
	}

	length = len[1];
	str = new char[length];

	memcpy(str, data + 1, len[1]);
	delete len;
	delete data;
}

NetString::~NetString() {
	delete this->str;
}

std::string NetString::string() const {
	return std::string{this->length} + std::string{this->str, (size_t)this->length};
}
