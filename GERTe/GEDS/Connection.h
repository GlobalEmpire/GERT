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
	Connection(SOCKET, std::string);
	Connection(SOCKET);

	void error(char * err);

public:
	virtual ~Connection();

	SOCKET sock;
	unsigned char state = 0;
	char vers[2];

	virtual void process() = 0;
	virtual void close() = 0;

	char * read(int=1);
};
