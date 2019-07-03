#pragma once
#include "INet.h"

class Server : public INet
{
public:
	enum class Type {
		GATEWAY,
		PEER
	};

	Server::Type type;			// Determines which type of connection is constructed when processed

	Server(short, Server::Type);
	~Server();

	void start();				// Begin listening to new connections - Prevents premature connections
	void process() override;	// Implement the INet event processing
};

