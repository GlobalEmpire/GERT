#pragma once
#ifdef _WIN32
static unsigned long nonZero = 1;
#endif
#include <string>

class Connection {
protected:
	~Connection();
	void error(char * err);

public:
	SOCKET * sock;
	unsigned char state = 0;
	char vers[2];

	Connection(SOCKET*, std::string);
	Connection(SOCKET*);

	char * read(int=1);
};
