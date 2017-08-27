/*
 * Manages peers and their associated connections for mapping use.
 */

#include <fcntl.h>
#include "peerManager.h"
#include "netty.h"
#include "libLoad.h"
#include "logging.h"

extern map<ipAddr, Peer*> peers;
extern map<ipAddr, KnownPeer> peerList;

extern bool running;

bool peerIter::isEnd() { return (ptr == peers.end()) || (ptr == ++peers.end()); }
peerIter peerIter::operator++ (int a) { return (ptr++, *this); }
peerIter::peerIter() : ptr(peers.begin()) {};
Peer* peerIter::operator*() { return ptr->second; }

bool knownIter::isEnd() { return ptr == peerList.end(); }
knownIter knownIter::operator++ (int a) { return (ptr++, *this); }
knownIter::knownIter() : ptr(peerList.begin()) {};
KnownPeer knownIter::operator*() { return ptr->second; }

void peerWatcher() {
	for (peersPtr iter = peers.begin(); iter != peers.end(); iter++) {
		Peer* target = iter->second;
		if (target == nullptr || target->sock == nullptr) {
			iter = peers.erase(iter);
			error("Null pointer in peer map");
			continue;
		}
		if (recv(*(SOCKET*)(target->sock), nullptr, 0, MSG_PEEK) == 0 || errno == ECONNRESET) {
			target->close();
		}
	}
}

Peer* lookup(ipAddr target) {
	return peers.at(target);
}

void _raw_peer(ipAddr key, Peer* value) {
	peers[key] = value;
}

void addPeer(ipAddr addr, portComplex ports) {
	peerList[addr] = KnownPeer(addr, ports);
	if (running)
		log("New peer " + addr.stringify());
}

void removePeer(ipAddr addr) {
	map<ipAddr, KnownPeer>::iterator iter = peerList.find(addr);
	peerList.erase(iter);
	log("Removed peer " + addr.stringify());
}

void sendToPeer(ipAddr addr, string data) {
	peers[addr]->transmit(data);
}

void broadcast(string data) {
	for (peerIter iter; !iter.isEnd(); iter++) {
		(*iter)->transmit(data);
	}
}
