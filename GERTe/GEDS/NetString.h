#include "Connection.h"

class NetString {
public:
	NetString(Connection*, int);
	~NetString();

	std::string string() const;
	unsigned char length;
	char * str;
};
