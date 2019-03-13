#ifdef _WIN32
typedef int socklen_t;
#else
#include <sys/socket.h>
#endif

#include "routeManager.h"
#include "logging.h"
#include <fcntl.h>
#include "Poll.h"
#include "GERTc.h"
#include "NetString.h"
#include "gatewayManager.h"
#include "peerManager.h"
#include "Versioning.h"

using namespace std;

map<IP, Peer*> peers;
map<IP, Ports> peerList;

extern Poll clientPoll;
extern volatile bool running;

enum Commands : char {
	REGISTERED,
	UNREGISTERED,
	ROUTE,
	RESOLVE,
	UNRESOLVE,
	LINK,
	UNLINK,
	CLOSEPEER,
	QUERY
};

Peer::Peer(SOCKET newSocket) : Connection(newSocket, "Peer") { //Incoming Peer Constructor
	sockaddr_in remoteip;
	socklen_t iplen = sizeof(sockaddr);
	getpeername(newSocket, (sockaddr*)&remoteip, &iplen);
	IP ip{ remoteip.sin_addr };

	if (peerList.count(ip) == 0) {
		char err[3] = { 0, 0, 1 }; //STATUS ERROR NOT_AUTHORIZED
		error(err);
		throw 1;
	}

	peers[ip] = this;
	log("Peer connected from " + ip.stringify());
	transmit(string({ (char)ThisVersion.major, (char)ThisVersion.minor }));

	if (vers[1] == 0)
		transmit("\0");
};

Peer::~Peer() { //Peer destructor
	killAssociated(this);
	peers.erase(ip);

	clientPoll.remove(sock);

	log("Peer " + ip.stringify() + " disconnected");
}

Peer::Peer(SOCKET socket, IP source) : Connection(socket), ip(source) { //Outgoing peer constructor
	peers[ip] = this;
}

void Peer::close() {
	transmit(string{ Commands::CLOSEPEER }); //SEND CLOSE REQUEST
	delete this;
}

void Peer::transmit(string data) {
	send(this->sock, data.c_str(), (ULONG)data.length(), 0);
}

void Peer::process() {
	if (state == 0) {
		state = 1;
		
		/*
			* Initial packet
			* MAJOR VERSION
			* MINOR VERSION
			*/
		return;
	}

	char * data = read(1);
	Commands command = (Commands)data[1];

	delete data;

	switch (command) {
	case ROUTE: {
		GERTc target = GERTc::extract(this);
		GERTc source = GERTc::extract(this);
		NetString data = NetString::extract(this);
		string cmd = { (char)Commands::ROUTE };
		cmd += target.tostring() + source.tostring() + data.string();
		if (!Gateway::sendTo(target.external, cmd)) {
			string errCmd = { UNREGISTERED };
			errCmd += target.tostring();
			this->transmit(errCmd);
		}
		return;
	}
	case REGISTERED: {
		Address target = Address::extract(this);
		RGateway * newGate = new RGateway{ target, this };
		return;
	}
	case UNREGISTERED: {
		Address target = Address::extract(this);
		delete RGateway::lookup(target);
		return;
	}
	case RESOLVE: {
		Address target = Address::extract(this);
		Key key = Key::extract(this);
		Key::add(target, key);
		return;
	}
	case UNRESOLVE: {
		Address target = Address::extract(this);
		Key::remove(target);
		return;
	}
	case LINK: {
		IP target = IP::extract(this);
		Ports ports = Ports::extract(this);
		allow(target, ports);
		return;
	}
	case UNLINK: {
		IP target = IP::extract(this);
		deny(target);
		return;
	}
	case CLOSEPEER: {
		this->transmit(string({ CLOSEPEER }));
		delete this;
		return;
	}
	case QUERY: {
		Address target = Address::extract(this);
		if (Gateway::lookup(target)) {
			string cmd = { REGISTERED };
			cmd += target.tostring();
			Gateway::sendTo(target, cmd);
		}
		else {
			string cmd = { UNREGISTERED };
			cmd += target.tostring();
			this->transmit(cmd);
		}
		return;
	}
	}
}

void Peer::allow(IP target, Ports bindings) {
	peerList[target] = bindings;
	if (running)
		log("New peer " + target.stringify());
}

void Peer::deny(IP target) {
	peerList.erase(target);
	log("Removed peer " + target.stringify());
}

void Peer::broadcast(std::string msg) {
	for (map<IP, Peer*>::iterator iter = peers.begin(); iter != peers.end(); iter++) {
		iter->second->transmit(msg);
	}
}
