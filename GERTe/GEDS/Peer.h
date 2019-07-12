#pragma once
#include "IP.h"
#include "Ports.h"

#undef REGISTERED

namespace GEDS {
	enum class Commands : char {
		REGISTERED,
		UNREGISTERED,
		ROUTE,
		RESOLVE,
		UNRESOLVE,
		LINK,
		UNLINK,
		CLOSE,
		QUERY
	};
}

class Peer : public Connection {
	IP ip;

public:
	Peer(SOCKET);
	Peer(SOCKET, IP);
	Peer(IP, unsigned short);					// Outgoing peer constructor
	~Peer();
	void close();
	void transmit(std::string);
	void process();

	static void allow(IP, Ports);
	static void deny(IP);
	static void broadcast(std::string);
};
