#include "Address.h"

Address::Address(const std::string &data) : addr{(unsigned char)data[0], (unsigned char)data[1], (unsigned char)data[2]} {};

Address::Address(const unsigned char* arr) : addr{arr[0], arr[1], arr[2]} {};

Address::Address(Connection * conn) {
	char * data = conn->read(3);
	
	addr[0] = data[1];
	addr[1] = data[2];
	addr[2] = data[3];

	delete data;
}

bool Address::operator== (const Address &target) const {
	bool first = (target.addr[0] == addr[0]);
	bool second = (target.addr[1] == addr[1]);
	bool third = (target.addr[2] == addr[2]);
	return first && second && third;
}

bool Address::operator< (const Address &target) const {
	if (addr[0] < target.addr[0])
		return true;
	else if (addr[0] == target.addr[0]) {
		if (addr[1] < target.addr[1])
			return true;
		else if (addr[1] == target.addr[1]) {
			if (addr[2] < target.addr[2])
				return true;
		}
	}
	return false;
}

const unsigned char* Address::getAddr() const {
	return addr;
}

std::string Address::stringify() const {
	unsigned short high, low;
	high = (unsigned short)(addr[0]) << 4 | (unsigned short)(addr[1]) >> 4;
	low = ((unsigned short)addr[1] & 0x0F) << 8 | (unsigned short)addr[2];
	return std::to_string(high) + "." + std::to_string(low);
}

std::string Address::tostring() const {
	return std::string{(char*)addr, 3};
}
