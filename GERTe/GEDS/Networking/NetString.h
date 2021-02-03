#include "Connection.h"

class NetString {
public:
	NetString(char, char*);
	~NetString();

	NetString static extract(Connection*);
	std::string string() const;
	char length;
	char * str;
};
