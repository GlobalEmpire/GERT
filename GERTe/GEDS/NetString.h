#include "Connection.h"

class NetString {
public:
	~NetString();

	NetString static extract(Connection*);
	std::string string() const;
	char length;
	char * str;
};
