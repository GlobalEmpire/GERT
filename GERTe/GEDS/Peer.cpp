#ifdef _WIN32
typedef int socklen_t;
#include <Ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
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
#include "Error.h"
#include <map>

using namespace std;

map<IP, Peer*> peers;
map<IP, Ports> peerList;

extern Poll netPoll;
extern volatile bool running;
extern std::map<Address, RGateway*> remotes;

constexpr unsigned int iplen = sizeof(sockaddr);

Peer::Peer(SOCKET newSocket) : Connection(newSocket, "Peer") { //Incoming Peer Constructor
	sockaddr_in remoteip;
	socklen_t temp = iplen;
	getpeername(newSocket, (sockaddr*)&remoteip, &temp);
	ip = IP{ remoteip.sin_addr };

	if (peerList.count(ip) == 0) {
		char err[3] = { 0, 0, 1 }; //STATUS ERROR NOT_AUTHORIZED
		error(err);
		::error("Unauthorized peer attempted to connect");
		throw 1;
	}

	peers[ip] = this;
	log("Peer connected from " + ip.stringify());

	last = (char)GEDS::Commands::CLOSE;											// Needs something that never calls consume
};

Peer::~Peer() { //Peer destructor
	RGateway::clean(this);
	peers.erase(ip);

	netPoll.remove(sock);

	log("Peer " + ip.stringify() + " disconnected");
}

Peer::Peer(IP target, unsigned short port) : ip(target) {				// Outgoing peer constructor
	sock = socket(AF_INET, SOCK_STREAM, 0);
	in_addr remoteIP = ip.addr;
	sockaddr_in addrFormat;
	addrFormat.sin_addr = remoteIP;
	addrFormat.sin_port = port;
	addrFormat.sin_family = AF_INET;

	// Correct excessive timeout period
#ifndef _WIN32
	int opt = 3;
	setsockopt(sock, IPPROTO_TCP, TCP_SYNCNT, (void*)& opt, sizeof(opt));
#else
	int opt = 2;
	setsockopt(sock, IPPROTO_TCP, TCP_MAXRT, (char*)& opt, sizeof(opt));
#endif

	int result = connect(sock, (sockaddr*)& addrFormat, iplen);

	if (result != 0) {
		warn(socketError("Failed to connect to " + ip.stringify() + ": "), false);
		throw 1;
	}

	peers[ip] = this;

	transmit(ThisVersion.tostring());

	char death[3];
	timeval timeout = { 2, 0 };

	setsockopt(sock, IPPROTO_TCP, SO_RCVTIMEO, (const char*)& timeout, sizeof(timeval));
	int result2 = recv(sock, death, 2, 0);

	if (result2 == -1) {
		::error("Connection to " + ip.stringify() + " dropped during negotiation");
		throw 1;
	}

	if (death[0] == 0) {
		char cause;
		recv(sock, &cause, 1, 0);

		if (cause == 0)
			warn("Peer " + ip.stringify() + " doesn't support " + ThisVersion.stringify());
		else if (cause == 1)
			::error("Peer " + ip.stringify() + " rejected this IP!");
		else
			::error("Peer " + ip.stringify() + " rejected this connection with an unknown error");

		throw 1;
	}

	vers[0] = death[0];
	vers[1] = death[1];

	state = 1;
	log("Connected to " + ip.stringify());

	setopts();

	netPoll.add(this);
}

void Peer::close() {
	transmit(string{ (char)GEDS::Commands::CLOSE }); //SEND CLOSE REQUEST
	delete this;
}

void Peer::transmit(string data) {
	send(this->sock, data.c_str(), (ULONG)data.length(), 0);
}

void Peer::process() {
	if (state == 0) {
		if (negotiate("Peer")) {
			transmit(string({ (char)ThisVersion.major, (char)ThisVersion.minor }));

			if (vers[1] == 0)
				transmit("\0");

			state = 1;
		}

		return;
	}

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

			new RGateway{ target, this };

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
