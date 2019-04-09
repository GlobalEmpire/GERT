#include "Connection.h"

class NetString {
public:
	NetString(Connection*);
	~NetString();

	std::string string() const;
	char length;
	char * str;
};
