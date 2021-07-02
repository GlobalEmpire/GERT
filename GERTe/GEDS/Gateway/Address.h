#pragma once
#include "../Networking/Connection.h"

class Address {
	unsigned char addr[3];

public:
	bool operator== (const Address&) const;
	bool operator!= (const Address&) const;

	bool operator< (const Address&) const;
	explicit Address(const std::string&);
	Address() : addr{0, 0, 0} {};
	Address static extract(Connection*);
	[[nodiscard]] const unsigned char* getAddr() const;
	[[nodiscard]] std::string stringify() const;
	[[nodiscard]] std::string tostring() const;
};

typedef Address GERTeAddr;
typedef Address GERTiAddr;