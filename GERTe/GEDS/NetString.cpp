#include "NetString.h"
#include <cstring>

NetString::~NetString() {
	delete this->str;
}

NetString NetString::extract(Connection* conn) {
	char * len = conn->read(1);
	char * data = conn->read(len[1]);

	if (data[0] < len[1]) {
		len[1] = data[0];
	}

	NetString string = {
			len[1],
			new char[len[1]]
	};

	memcpy(&string.str, data+1, len[1]);
	delete len;
	delete data;

	return string;
}

std::string NetString::string() const {
	return std::string{this->length} + std::string{this->str, (size_t)this->length};
}
