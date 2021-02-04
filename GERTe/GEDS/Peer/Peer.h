#pragma once
#include "../Networking/IP.h"
#include "../Networking/Ports.h"

class Peer : public Connection {
	IP ip;

public:
	Peer(SOCKET);
	~Peer();
	Peer(SOCKET, IP);
	void close();
	void process();

	static void allow(IP, Ports);
	static void deny(IP);
	static void broadcast(const std::string&);
};
