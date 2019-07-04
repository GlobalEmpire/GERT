#pragma once
#include "IConsumer.hpp"
#include <string>

class Connection : public IConsumer
{
protected:
	char last = 0;

	Connection(SOCKET, std::string);
	Connection(SOCKET);

	void error(char * err);

public:
	virtual ~Connection();

	unsigned char state = 0;
	char vers[2];

	virtual void close() = 0;
};
