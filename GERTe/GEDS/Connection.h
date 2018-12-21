#pragma once

#ifdef _WIN32
static unsigned long nonZero = 1;
#endif

class Connection {
public:
	void * sock;
	unsigned char state = 0;
	Connection(void * socket) : sock(socket) {};
	char * read(int=1);
};
