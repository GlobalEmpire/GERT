#ifndef _WIN32
#include <unistd.h>
#endif
#include <thread>
#include "Gateway.h"
#include "logging.h"
#include "Peer.h"
#include "Poll.h"
#include "Error.h"
#include "Processor.h"
#include "Server.h"
#include <map>
using namespace std;

Server* gateServer;
Server* peerServer;

Poll netPoll;

Processor* proc;

extern volatile bool running;
extern unsigned short gatewayPort;
extern unsigned short peerPort;
extern char * LOCAL_IP;
extern vector<UGateway*> noAddrList;
extern map<IP, Peer*> peers;
extern std::map<Address, Gateway*> gateways;
extern map<IP, Ports> peerList;

void killConnections() {
	for (std::pair<Address, Gateway*> pair : gateways) {
		pair.second->close();
		delete pair.second;
	}
	for (std::pair<IP, Peer*> pair : peers) {
		pair.second->close();
		delete pair.second;
	}
	for (UGateway* gate : noAddrList) {
		gate->close();
		delete gate;
	}
}

//PUBLIC
void startup() {
	//Server construction
#ifdef _WIN32 //If compiled for Windows
	WSADATA socketConfig; //Construct WSA configuration destination
	int res = WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock

	if (res != 0) {
		knownError(res, "Cannot start networking: ");
		crash(ErrorCode::LIBRARY_ERROR);
	}
#endif

	gateServer = new Server{ gatewayPort, Server::Type::GATEWAY };
	peerServer = new Server{ peerPort, Server::Type::PEER };

	netPoll.add(gateServer);
	netPoll.add(peerServer);
}

//PUBLIC
void cleanup() {
	delete gateServer;
	delete peerServer;

	killConnections();
}

//PUBLIC
void runServer() { //Listen for new connections
	debug("Starting message processor");
	proc = new Processor{ &netPoll };

	debug("Starting connection processor");
	gateServer->start();
	peerServer->start();

	debug("Main thread going to sleep");
#ifdef _WIN32
	SuspendThread(GetCurrentThread());
#else
	pause();
#endif
}

void buildWeb() {
	for (map<IP, Ports>::iterator iter = peerList.begin(); iter != peerList.end(); iter++) {
		IP ip = iter->first;
		Ports ports = iter->second;

		if (ip.stringify() == LOCAL_IP)
			continue;
		else if (ports.peer == 0) {
			debug("Skipping peer " + ip.stringify() + " because its outbound only.");
			continue;
		}

		try {
			Peer newPeer{ ip, ports.peer };
		}
		catch ([[maybe_unused]] int e) {}
	}
}
