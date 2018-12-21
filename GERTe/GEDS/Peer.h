#pragma once
#include "KnownPeer.h"
#include "Connection.h"

class Peer : public Connection {
	friend int emergencyScan();
	KnownPeer * id;
public:
	Peer(void *);
	~Peer();
	Peer(void*, Version*, KnownPeer*);
	void close();
	void transmit(std::string);
};
