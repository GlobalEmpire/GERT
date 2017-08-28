#include "KnownPeer.h"
#include <map>

map<IP, KnownPeer> peerList;

KnownPeer * getKnown(sockaddr_in addr) {
	IP formatted = addr.sin_addr;
	if (peerList.count(formatted) == 0)
		return nullptr;
	return &peerList[formatted];
}
