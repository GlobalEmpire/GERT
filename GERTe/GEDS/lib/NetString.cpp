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

string NetString::tostring() const {
	return string{this->length} + string{this->str, this->length};
}
