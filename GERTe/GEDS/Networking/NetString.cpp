#include "NetString.h"
#include <cstring>
#include <utility>

NetString::NetString(std::string str) : data(std::move(str)) {}

NetString NetString::extract(Connection* conn) {
	unsigned short rawlen = *(unsigned short*)conn->read(2).c_str();
	std::string data = conn->read(rawlen);

	return { data };
}
