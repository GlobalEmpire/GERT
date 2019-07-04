#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <Ws2tcpip.h>
#else
#include <sys/socket.h> //Load C++ standard socket API
#include <netinet/tcp.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#endif
#include <sys/types.h>
#include <thread>
#include "query.h"
#include "Gateway.h"
#include "logging.h"
#include "Peer.h"
#include "Poll.h"
#include "Versioning.h"
#include "Error.h"
#include "Processor.h"
#include "Server.h"
#include <map>
using namespace std;

Server* gateServer;
Server* peerServer;

Poll serverPoll;
Poll clientPoll;

Processor* proc;

extern volatile bool running;
extern char * gatewayPort;
extern char * peerPort;
extern char * LOCAL_IP;
extern vector<UGateway*> noAddrList;
extern map<IP, Peer*> peers;
extern std::map<Address, Gateway*> gateways;
extern map<IP, Ports> peerList;

constexpr unsigned int iplen = sizeof(sockaddr);

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

	gateServer = new Server{ (short)std::stoi(gatewayPort), Server::Type::GATEWAY };
	peerServer = new Server{ (short)std::stoi(peerPort), Server::Type::PEER };

	serverPoll.add(gateServer->sock, gateServer);
	serverPoll.add(peerServer->sock, peerServer);
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
	proc = new Processor{ &clientPoll };

	debug("Starting connection processor");
	gateServer->start();
	peerServer->start();

	while (running) { //Dies on SIGINT
		Event_Data data = serverPoll.wait();

		if (data.fd == 0) {
			return;
		}
		else if (data.fd == gateServer->sock) {
			gateServer->process();
		}
		else {
			peerServer->process();
		}

#ifdef _WIN32
		serverPoll.remove(data.fd);
		serverPoll.add(data.fd, data.ptr);
#endif
	}
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

		SOCKET newSock = socket(AF_INET, SOCK_STREAM, 0);
		in_addr remoteIP = ip.addr;
		sockaddr_in addrFormat;
		addrFormat.sin_addr = remoteIP;
		addrFormat.sin_port = ports.peer;
		addrFormat.sin_family = AF_INET;

#ifndef _WIN32
		int opt = 3;
		setsockopt(newSock, IPPROTO_TCP, TCP_SYNCNT, (void*)&opt, sizeof(opt)); //Correct excessive timeout period on Linux
#else
		int opt = 2;
		setsockopt(newSock, IPPROTO_TCP, TCP_MAXRT, (char*)&opt, sizeof(opt)); //Correct excessive timeout period on Windows
#endif

		int result = connect(newSock, (sockaddr*)&addrFormat, iplen);

		if (result != 0) {
			warn("Failed to connect to " + ip.stringify() + " " + to_string(errno));
			continue;
		}

		Peer * newConn = new Peer(newSock, ip);

		newConn->transmit(ThisVersion.tostring());

		char death[3];
		timeval timeout = { 2, 0 };

		setsockopt(newSock, IPPROTO_TCP, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeval));
		int result2 = recv(newSock, death, 2, 0);

		if (result2 == -1) {
			delete newConn;
			error("Connection to " + ip.stringify() + " dropped during negotiation");
			continue;
		}

		if (death[0] == 0) {
			recv(newSock, death + 2, 1, 0);

			if (death[2] == 0) {
				warn("Peer " + ip.stringify() + " doesn't support " + ThisVersion.stringify());
				delete newConn;
				continue;
			}
			else if (death[2] == 1) {
				error("Peer " + ip.stringify() + " rejected this IP!");
				delete newConn;
				continue;
			}
		}

		newConn->state = 1;
		log("Connected to " + ip.stringify());

		clientPoll.add(newSock, newConn);
	}
}
