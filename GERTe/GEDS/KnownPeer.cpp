#include "KnownPeer.h"
#include <map>

map<ipAddr, KnownPeer> peerList;

KnownPeer * getKnown(sockaddr_in addr) {
	ipAddr formatted = addr.sin_addr;
	if (peerList.count(formatted) == 0)
		return nullptr;
	return &peerList[formatted];
}
