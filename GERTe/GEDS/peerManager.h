#ifndef __PEER_MNGR__
#define __PEER_MNGR__
#include "netDefs.h"
#include "Peer.h"
#include <map>

typedef map<ipAddr, Peer*>::iterator peersPtr;
typedef map<ipAddr, KnownPeer>::iterator knownPtr;

class peerIter {
	peersPtr ptr;
	public:
		bool isEnd();
		peerIter operator++(int);
		peerIter();
		Peer* operator*();
};

class knownIter {
	knownPtr ptr;
	public:
		bool isEnd();
		knownIter operator++(int);
		knownIter();
		KnownPeer operator*();
};

extern "C" {
	void peerWatcher();
	Peer* lookup(ipAddr);
	void _raw_peer(ipAddr, Peer*);
	void addPeer(ipAddr, portComplex);
	void removePeer(ipAddr);
	void sendToPeer(ipAddr, string);
	void broadcast(string);
	void initPeer(void*);
}
#endif
