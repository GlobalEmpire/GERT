#pragma once
#include "IP.h"
#include "Peer.h"
#include <map>
#include "Ports.h"

typedef std::map<IP, Peer*>::iterator peersPtr;

class peerIter {
	peersPtr ptr;
	public:
		bool isEnd();
		peerIter operator++(int);
		peerIter();
		Peer* operator*();
};
