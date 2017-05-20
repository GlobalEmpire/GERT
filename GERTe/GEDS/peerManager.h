#ifndef __PEER_MNGR__
#define __PEER_MNGR__
#include "netDefs.h"
#include <map>

typedef map<ipAddr, peer*>::iterator peersPtr;
typedef map<ipAddr, knownPeer>::iterator knownPtr;

class peerIter {
	peersPtr ptr;
	public:
		bool isEnd();
		peerIter operator++(int);
		peerIter();
		peer* operator*();
};

class knownIter {
	knownPtr ptr;
	public:
		bool isEnd();
		knownIter operator++(int);
		knownIter();
		knownPeer operator*();
};

void peerWatcher();
void initPeer(void*);
peer* lookup(ipAddr);
void _raw_peer(ipAddr, peer*);
void closeTarget(peer*);
void addPeer(ipAddr, portComplex);
void removePeer(ipAddr);
void sendTo(ipAddr, string);
void broadcast(string);
void initPeer(void*);
#endif
