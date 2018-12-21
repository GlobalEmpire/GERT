#include "Connection.h"
#include <string>

class NetString {
public:
	~NetString();

	NetString static extract(Connection*);
	std::string string() const;
	char length;
	char * str;
};
