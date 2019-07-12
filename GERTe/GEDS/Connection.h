#pragma once
#include "IConsumer.hpp"
#include <string>

class Connection : public IConsumer
{
protected:
	char last = 0;

	Connection(SOCKET, std::string);		// Generic incoming connection constructor
	Connection();							// Connection constructor which allows the derived class to defer socket acquisition.

	void setopts();							// Sets the connection's socket options. This is done in the constructor, but certain derived classes may require it anyways.

	void error(char * err);

public:
	virtual ~Connection();

	unsigned char state = 0;
	char vers[2] = { 0,0 };

	virtual void close() = 0;

	bool negotiate(std::string);			// Negotiates the new connection. Returns true is negotiated. Returns false if it needs more data or fails. Cleans up on failure.
};
