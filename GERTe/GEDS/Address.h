#pragma once
#include "Connection.h"

class Address {
	unsigned char addr[3];

public:
	bool operator== (const Address&) const;
	bool operator< (const Address&) const;
	Address(const std::string&);
	Address(const unsigned char*);
	Address() : addr{0, 0, 0} {};
	Address(Connection*, int);

	const unsigned char* getAddr() const;
	std::string stringify() const;
	std::string tostring() const;
};

typedef Address GERTeAddr;
typedef Address GERTiAddr;