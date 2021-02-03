#include "Address.h"

class GERTc {
public:
	Address external;
	Address internal;
	GERTc(const std::string& data) : external(data.substr(0, 3)), internal(data.substr(3, 3)) {};
	GERTc(const unsigned char* ptr) : external(ptr), internal(ptr + 3) {};
	GERTc() : external(), internal() {};
	std::string stringify() const { return external.stringify() + ":" + internal.stringify(); }
	std::string tostring() const { return external.tostring() + internal.tostring(); }
	GERTc static extract(Connection * conn) {
		GERTc addr;
		addr.external = Address::extract(conn);
		addr.internal = Address::extract(conn);

		return addr;
	};
};