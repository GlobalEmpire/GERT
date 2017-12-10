#pragma once
#include "Connection.h"
using namespace std;

class Address {
	unsigned char addr[3];

public:
	bool operator== (const Address&) const;
	bool operator< (const Address&) const;
	Address(const string&);
	Address(const unsigned char*);
	Address() : addr{0, 0, 0} {};
	Address static extract(Connection*);
	const unsigned char* getAddr() const;
	string stringify() const;
	string tostring() const;
};

typedef Address GERTeAddr;
typedef Address GERTiAddr;
