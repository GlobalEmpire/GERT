#pragma once
#include "Version.h"

#ifdef _WIN32
static unsigned long nonZero = 1;
#endif

class Connection {
public:
	void * sock;
	Version * api;
	unsigned char state = 0;
	Connection(void * socket, Version * vers = nullptr) : sock(socket), api(vers) {};
	char * read(int=1);
};
