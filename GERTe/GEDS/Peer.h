#include "KnownPeer.h"
#include "Connection.h"

class Peer : public Connection {
	friend int emergencyScan();
	KnownPeer * id;
public:
	Peer(void *);
	Peer(void*, Version*, KnownPeer*);
	void process(string data) { api->procPeer(this, data); }
	void kill() { api->killPeer(this); }
	void close();
	void transmit(string);
};
