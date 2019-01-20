#pragma once
#ifdef _WIN32
static unsigned long nonZero = 1;
#endif
#include <string>

class Connection {
public:
	void * sock;
	unsigned char state = 0;
	char vers[2];

	Connection(void*, std::string);
	Connection(void*);

	char * read(int=1);
	void error(char*);
};
