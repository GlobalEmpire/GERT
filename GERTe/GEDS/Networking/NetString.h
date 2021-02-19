#include "Connection.h"

class NetString {
public:
	NetString(std::string);

	NetString static extract(Connection*);
	std::string data;
};
