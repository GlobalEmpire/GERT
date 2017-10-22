#include "Peer.h"
#include "libLoad.h"
#include "netty.h"
#include "routeManager.h"
#include "logging.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <map>

map<IP, Peer*> peers;

void sockError(SOCKET * sock, char * err, Peer* me) {
	send(*sock, err, 3, 0);
	destroy(sock);
	throw 1;
}

Peer::Peer(void * sock) : Connection(sock) {
	SOCKET * newSocket = (SOCKET*)sock;
	char buf[3];
	recv(*newSocket, buf, 3, 0);
	log((string)"GEDS using " + to_string(buf[0]) + "." + to_string(buf[1]) + "." + to_string(buf[2]));
	UCHAR major = buf[0]; //Major version number
	api = getVersion(major); //Find API version
#ifdef _WIN32
	ioctlsocket(*newSocket, FIONBIO, &nonZero);
#else
	fcntl(*newSocket, F_SETFL, O_NONBLOCK);
#endif
	if (api == nullptr) { //Determine if major number is not supported
		char error[3] = { 0, 0, 0 };
		sockError(newSocket, error, this); //This is me :D
	}
	else { //Major version found
		sockaddr remotename;
		getpeername(*newSocket, &remotename, (unsigned int*)&iplen);
		sockaddr_in remoteip = *(sockaddr_in*)&remotename;
		id = getKnown(remoteip);
		if (id == nullptr) {
			char error[3] = { 0, 0, 1 }; //STATUS ERROR NOT_AUTHORIZED
			sockError(newSocket, error, this);
		}
		peers[id->addr] = this;
		log("Peer connected from " + id->addr.stringify());
		this->process();
	}
};

Peer::Peer(void * socket, Version * vers, KnownPeer * known) : Connection(socket, vers), id(known) {
	peers[id->addr] = this;
}

void Peer::close() {
	killAssociated(this);
	peers.erase(this->id->addr);
	destroy((SOCKET*)this->sock);
	log("Peer " + this->id->addr.stringify() + " disconnected");
	delete this;
}

void Peer::transmit(string data) {
	send(*(SOCKET*)this->sock, data.c_str(), (ULONG)data.length(), 0);
}
