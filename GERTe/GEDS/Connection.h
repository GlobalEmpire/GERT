#pragma once
#include <string>

#ifdef _WIN32
typedef unsigned long long SOCKET;
static unsigned long nonZero = 1;
#else
typedef int SOCKET;
#endif

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
