#pragma once
#include "IP.h"
#include "Peer.h"
#include <map>
#include "Ports.h"

typedef std::map<IP, Peer*>::iterator peersPtr;
typedef std::map<IP, KnownPeer>::iterator knownPtr;

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
