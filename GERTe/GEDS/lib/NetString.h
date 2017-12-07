#include "../Connection.h"
#include <string>

class NetString {
public:
	~NetString();

	NetString static extract(Connection*);
	string tostring() const;
	char length;
	char * str;
};
