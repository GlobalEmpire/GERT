#pragma once
#include <string>
#include "INet.h"

class Connection : public INet
{
protected:
	Connection(SOCKET, std::string);
	Connection(SOCKET);

	void error(char * err);

public:
	virtual ~Connection();

	unsigned char state = 0;
	char vers[2];

	virtual void close() = 0;

	char * read(unsigned char=1);
};
