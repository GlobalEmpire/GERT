#include <string>
using namespace std;

class Address {
	unsigned char addr[3];

public:
	bool operator== (const Address&) const;
	bool operator< (const Address&) const;
	Address(const string&);
	Address(const unsigned char*);
	Address() : addr{0, 0, 0} {};
	const unsigned char* getAddr() const;
	string stringify() const;
};

typedef Address GERTeAddr;
typedef Address GERTiAddr;
