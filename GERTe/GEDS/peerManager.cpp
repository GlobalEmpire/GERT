/*
 * Manages peers and their associated connections for mapping use.
 */

#ifndef _WIN32
#include <fcntl.h>
typedef int SOCKET;
#endif

#include <map>
#include "netDefs.h"
#include "peerManager.h"
#include "netty.h"
#include "routeManager.h"
#include "logging.h"
#include "libLoad.h"

typedef map<ipAddr, peer*>::iterator peersPtr;
typedef map<ipAddr, knownPeer>::iterator authPtr;

map<ipAddr, peer*> peers;
map<ipAddr, knownPeer> peerList;

extern bool running;

#ifdef _WIN32
const int iplen = 16;
#else
const unsigned int iplen = 16;
#endif

bool peerIter::isEnd() { return ptr == peers.end(); }
peerIter peerIter::operator++ (int a) { return (ptr++, *this); }
peerIter::peerIter() : ptr(peers.begin()) {};
peer* peerIter::operator*() { return ptr->second; }

bool knownIter::isEnd() { return ptr == peerList.end(); }
knownIter knownIter::operator++ (int a) { return (ptr++, *this); }
knownIter::knownIter() : ptr(peerList.begin()) {};
knownPeer knownIter::operator*() { return ptr->second; }

void watcher() {
	for (peersPtr iter; iter != peers.end(); iter++) {
		peer* target = iter->second;
		if (target == nullptr || target->sock == nullptr) {
			ipAddr temp = iter++->first;
			peers.erase(iter->first);
			iter = peers.find(temp);
			continue;
		}
		if (recv(*(SOCKET*)(target->sock), nullptr, 0, MSG_PEEK) == 0 || errno == ECONNRESET) {
			closeTarget(target);
		}
	}
}

void closeTarget(peer* target) {
	//killAssociated(target);
	peers.erase(target->addr);
	destroy((SOCKET*)target->sock);
	log("Peer " + target->addr.stringify() + " disconnected");
}

void initPeer(void * newSock) {
	SOCKET * newSocket = (SOCKET*)newSock;
#ifdef _WIN32
	ioctlsocket(*newSocket, FIONBIO, &nonZero);
#else
	fcntl(*newSocket, F_SETFL, O_NONBLOCK);
#endif
	char buf[3];
	recv(*newSocket, buf, 3, 0);
	log((string)"GEDS using " + buf[0] + "." + buf[1] + "." + buf[2]);
	UCHAR major = buf[0]; //Major version number
	version* api = getVersion(major); //Find API version
	if (api == nullptr) { //Determine if major number is not supported
		char error[3] = { 0, 0, 0 };
		send(*newSocket, error, 3, 0); //Notify client we cannot serve this version
		destroy(newSocket); //Close the socket
	}
	else { //Major version found
		sockaddr remotename;
		getpeername(*newSocket, &remotename, (unsigned int*)&iplen);
		sockaddr_in remoteip = *(sockaddr_in*)&remotename;
		if (peerList.count(remoteip.sin_addr) == 0) {
			char error[3] = { 0, 0, 1 }; //STATUS ERROR NOT_AUTHORIZED
			send(*newSocket, error, 3, 0);
		}
		peer* newConnection = new peer(&newSocket, api, remoteip.sin_addr); //Create new connection
		peers[remoteip.sin_addr] = newConnection;
		log("Peer connected from " + newConnection->addr.stringify());
		newConnection->process("");
	}
}

peer* lookup(ipAddr target) {
	return peers.at(target);
}

void _raw_peer(ipAddr key, peer* value) {
	peers[key] = value;
}

void addPeer(ipAddr addr, portComplex ports) {
	peerList[addr] = knownPeer(addr, ports);
	if (running)
		log("New peer " + addr.stringify());
}

void removePeer(ipAddr addr) {
	map<ipAddr, knownPeer>::iterator iter = peerList.find(addr);
	peerList.erase(iter);
	log("Removed peer " + addr.stringify());
}

void sendTo(ipAddr addr, string data) {
	sendTo(peers[addr], data);
}

void broadcast(string data) {
	for (peerIter iter; !iter.isEnd(); iter++) {
		sendTo(*iter, data);
	}
}
