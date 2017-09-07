#include "Address.h"

class GERTc {

public:
	Address external;
	Address internal;
	GERTc(const string& data) : external(data.substr(0, 3)), internal(data.substr(3, 3)) {};
	GERTc(const unsigned char* ptr) : external(ptr), internal(ptr + 3) {};
	GERTc() : external(), internal() {};
	string stringify() const { return external.stringify() + ":" + internal.stringify(); }
};
