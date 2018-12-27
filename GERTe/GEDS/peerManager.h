#pragma once
#include "IP.h"
#include "Peer.h"
#include <map>

class peerIter {
	std::map<IP, Peer*>::iterator ptr;
	public:
		bool isEnd();
		peerIter operator++(int);
		peerIter();
		Peer* operator*();
};
