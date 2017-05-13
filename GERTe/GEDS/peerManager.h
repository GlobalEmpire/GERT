#include "netDefs.h"

class peerIter {
	public:
		bool isEnd();
		peerIter operator++(int);
		peerIter();
		peer* operator*();
};

void watcher();
void initPeer(SOCKET*);
peer* lookup(ipAddr);
void _raw_peer(ipAddr, peer*);
void closeTarget(peer*);
