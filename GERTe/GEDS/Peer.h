#include "KnownPeer.h"

class Peer : public connection {
	friend int emergencyScan();
	KnownPeer * id;
public:
	Peer(void *);
	Peer(void*, version*, KnownPeer*);
	void process(string data) { api->procPeer(this, data); }
	void kill() { api->killPeer(this); }
	void close();
	void transmit(string);
};
