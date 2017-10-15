#pragma once
#include "KnownPeer.h"
#include "Connection.h"

class Peer : public Connection {
	friend int emergencyScan();
	KnownPeer * id;
public:
	Peer(void *);
	Peer(void*, Version*, KnownPeer*);
	Peer(int);
	void process() { api->procPeer(this); }
	void kill() { api->killPeer(this); }
	void close();
	void transmit(string);
};
