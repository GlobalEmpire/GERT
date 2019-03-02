#pragma once
#include "IP.h"
#include "Ports.h"

class Peer : public Connection {
	friend int emergencyScan();
	IP ip;

public:
	Peer(SOCKET);
	~Peer();
	Peer(SOCKET, IP);
	void close();
	void transmit(std::string);
	void process();

	static void allow(IP, Ports);
	static void deny(IP);
	static void broadcast(std::string);
};
