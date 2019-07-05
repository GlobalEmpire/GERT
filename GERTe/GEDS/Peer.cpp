#ifdef _WIN32
typedef int socklen_t;
#else
#include <sys/socket.h>
#endif

#include "Peer.h"
#include "logging.h"
#include <fcntl.h>
#include "Poll.h"
#include "Gateway.h"
#include "GERTc.h"
#include "NetString.h"
#include "Versioning.h"
#include "RGateway.h"
#include <map>

using namespace std;

map<IP, Peer*> peers;
map<IP, Ports> peerList;

extern Poll netPoll;
extern volatile bool running;
extern std::map<Address, RGateway*> remotes;

Peer::Peer(SOCKET newSocket) : Connection(newSocket, "Peer") { //Incoming Peer Constructor
	sockaddr_in remoteip;
	socklen_t iplen = sizeof(sockaddr);
	getpeername(newSocket, (sockaddr*)&remoteip, &iplen);
	IP ip{ remoteip.sin_addr };

	if (peerList.count(ip) == 0) {
		char err[3] = { 0, 0, 1 }; //STATUS ERROR NOT_AUTHORIZED
		error(err);
		::error("Unauthorized peer attempted to connect");
		throw 1;
	}

	peers[ip] = this;
	log("Peer connected from " + ip.stringify());
	transmit(string({ (char)ThisVersion.major, (char)ThisVersion.minor }));

	if (vers[1] == 0)
		transmit("\0");

	last = (char)GEDS::Commands::CLOSE;											// Needs something that never calls consume
};

Peer::~Peer() { //Peer destructor
	RGateway::clean(this);
	peers.erase(ip);

	netPoll.remove(this);

	log("Peer " + ip.stringify() + " disconnected");
}

Peer::Peer(SOCKET socket, IP source) : Connection(socket), ip(source) { //Outgoing peer constructor
	peers[ip] = this;
}

void Peer::close() {
	transmit(string{ (char)GEDS::Commands::CLOSE }); //SEND CLOSE REQUEST
	delete this;
}

void Peer::transmit(string data) {
	send(this->sock, data.c_str(), (ULONG)data.length(), 0);
}

void Peer::process() {
	if (last == (char)GEDS::Commands::CLOSE) {
		consume(1);
		last = buf[0];
		clean();
	}

	GEDS::Commands command = (GEDS::Commands)last;

	switch (command) {
	case GEDS::Commands::ROUTE:
		if (consume(12, true)) {
			GERTc target = { this, 0 };
			GERTc source = { this, 6 };
			NetString data{ this, 13 };
			clean();

			string cmd = { (char)GEDS::Commands::ROUTE };
			cmd += target.tostring() + source.tostring() + data.string();
			if (!Gateway::sendTo(target.external, cmd)) {
				string errCmd = { (char)GEDS::Commands::UNREGISTERED };
				errCmd += target.tostring();
				transmit(errCmd);
			}

			last = (char)GEDS::Commands::CLOSE;
		}
		return;
	case GEDS::Commands::REGISTERED:
		if (consume(3)) {
			Address target{ this, 0 };
			clean();

			RGateway* newGate = new RGateway{ target, this };

			last = (char)GEDS::Commands::CLOSE;
		}
		return;
	case GEDS::Commands::UNREGISTERED:
		if (consume(3)) {
			Address target{ this, 0 };
			clean();

			delete RGateway::lookup(target);

			last = (char)GEDS::Commands::CLOSE;
		}
		return;
	case GEDS::Commands::RESOLVE:
		if (consume(23)) {
			Address target{ this, 0 };
			Key key{ this, 3 };
			clean();

			Key::add(target, key);

			last = (char)GEDS::Commands::CLOSE;
		}
		return;
	case GEDS::Commands::UNRESOLVE:
		if (consume(3)) {
			Address target{ this, 0 };
			clean();

			Key::remove(target);

			last = (char)GEDS::Commands::CLOSE;
		}
		return;
	case GEDS::Commands::LINK:
		if (consume(8)) {
			IP target{ this, 0 };
			Ports ports{ this, 4 };
			clean();

			allow(target, ports);

			last = (char)GEDS::Commands::CLOSE;
		}
		return;
	case GEDS::Commands::UNLINK:
		if (consume(4)) {
			IP target{ this, 0 };
			clean();

			deny(target);

			last = (char)GEDS::Commands::CLOSE;
		}
		return;
	case GEDS::Commands::CLOSE:
		transmit(string({ (char)GEDS::Commands::CLOSE }));
		delete this;
		return;
	case GEDS::Commands::QUERY:
		if (consume(3)) {
			Address target{ this, 0 };
			clean();

			if (Gateway::lookup(target)) {
				string cmd = { (char)GEDS::Commands::REGISTERED };
				cmd += target.tostring();
				Gateway::sendTo(target, cmd);
			}
			else {
				string cmd = { (char)GEDS::Commands::UNREGISTERED };
				cmd += target.tostring();
				transmit(cmd);
			}

			last = (char)GEDS::Commands::CLOSE;
		}
		return;
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
