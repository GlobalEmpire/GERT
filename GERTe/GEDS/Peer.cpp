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
#include "Random.h"

using namespace std;

map<IP, Peer*> peers;
map<IP, Ports> peerList;

extern Poll peerPoll;
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
	QUERY,
	TUNNEL_START,
	TUNNEL_OPEN,
	TUNNEL_DATA,
	TUNNEL_END
};

enum class GateCommands : char {
	STATE,
	REGISTER,
	DATA,
	CLOSE,
	TUNNEL_START,
	TUNNEL_DATA,
	TUNNEL_END
};

enum class GateStates : char {
	FAILURE,
	CONNECTED,
	REGISTERED,
	CLOSED,
	SENT,
	TUNNEL_STARTED
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

	peerPoll.remove(sock);

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
	case TUNNEL_START: {
		char* tunRaw = read(2);
		uint16_t remoteTun = ntohs((uint16_t)(tunRaw + 1));

		uint16_t tunNum = random();

		while (UGateway::tunnels.count(tunNum) != 0)
			tunNum = random();

		GERTc target = GERTc::extract(this);
		GERTc source = GERTc::extract(this);

		Tunnel tun{
			target,
			source,
			remoteTun
		};

		union {
			uint16_t num;
			char bytes[2];
		} netTun;

		netTun.num = ntohs(tunNum);
		string newCmd = string({ (char)GateCommands::TUNNEL_START, netTun.bytes[0], netTun.bytes[1] }) + target.tostring() + source.tostring();
		string response = string({ TUNNEL_OPEN, tunRaw[1], tunRaw[2], netTun.bytes[0], netTun.bytes[1] });

		if (Gateway::sendTo(target.external, newCmd)) {
			UGateway::tunnels[tunNum] = tun;
			transmit(response);
		}
		else {
			string errCmd({ TUNNEL_END, tunRaw[1], tunRaw[2] });
			transmit(errCmd);

			errCmd = { UNREGISTERED };
			errCmd += target.tostring();
			transmit(errCmd);
		}

		delete[] tunRaw;
		return;
	}
	case TUNNEL_OPEN: {
		char* tunRaw = read(4);
		uint16_t ourTun = ntohs((uint16_t)(tunRaw + 1));
		uint16_t remoteTun = ntohs((uint16_t)(tunRaw + 3));

		if (UGateway::tunnels.count(ourTun) == 0) {
			string errCmd({ TUNNEL_END, tunRaw[3], tunRaw[4] });
			transmit(errCmd);
		}
		else {
			UGateway::tunnels[ourTun].remoteId = remoteTun;
			string newCmd({ (char)GateCommands::STATE, (char)GateStates::TUNNEL_STARTED, tunRaw[1], tunRaw[2] });

			if (!Gateway::sendTo(UGateway::tunnels[ourTun].local.external, newCmd)) {
				UGateway::tunnels.erase(ourTun);

				string errCmd({ TUNNEL_END, tunRaw[3], tunRaw[4] });
				transmit(errCmd);
			}
		}

		delete[] tunRaw;
		return;
	}
	case TUNNEL_DATA: {
		char* tunRaw = read(2);
		uint16_t ourTun = ntohs((uint16_t)(tunRaw + 1));

		if (UGateway::tunnels.count(ourTun)) {
			NetString data = NetString::extract(this);
			string cmd = { (char)GateCommands::TUNNEL_DATA, tunRaw[1], tunRaw[2] };
			cmd += data.string();

			Address target = UGateway::tunnels[ourTun].local.external;

			if (!Gateway::sendTo(target, cmd)) {
				union {
					uint16_t num;
					char bytes[2];
				} netTun;

				netTun.num = ntohs(UGateway::tunnels[ourTun].remoteId);

				string errCmd({ TUNNEL_END, netTun.bytes[0], netTun.bytes[1] });
				transmit(errCmd);
				errCmd = { UNREGISTERED };
				errCmd += target.tostring();
				this->transmit(errCmd);
			}
		}
		else {
			// Well shit. We don't have the ID we need to close the tunnel.
		}

		delete[] tunRaw;
		return;
	}
	case TUNNEL_END: {
		char* tunRaw = read(2);
		uint16_t ourTun = ntohs((uint16_t)(tunRaw + 1));

		map<uint16_t, Tunnel>::iterator iter = UGateway::tunnels.find(ourTun);
		string newCmd({ (char)GateCommands::TUNNEL_END, tunRaw[1], tunRaw[2] });
		if (iter != UGateway::tunnels.end()) {
			Gateway::sendTo(iter->second.local.external, newCmd);
			UGateway::tunnels.erase(iter);
		}

		delete[] tunRaw;
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
