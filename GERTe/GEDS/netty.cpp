#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#else
#include <sys/socket.h> //Load C++ standard socket API
#include <netinet/tcp.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#endif
#include <sys/types.h>
#include <thread>
#include "peerManager.h"
#include "routeManager.h"
#include "gatewayManager.h"
#include "query.h"
#include "logging.h"
#include "Poll.h"
#include "Versioning.h"
using namespace std;

SOCKET gateServer, gedsServer; //Define both server sockets

Poll gatePoll;
Poll peerPoll;
Poll serverPoll;

extern volatile bool running;
extern char * gatewayPort;
extern char * peerPort;
extern char * LOCAL_IP;
extern vector<UGateway*> noAddrList;
extern map<IP, Ports> peerList;

constexpr unsigned int iplen = sizeof(sockaddr);

void killConnections() {
	for (gatewayIter iter; !iter.isEnd(); iter++) {
		(*iter)->close();
		delete *iter;
	}
	for (peerIter iter; !iter.isEnd(); iter++) {
		(*iter)->close();
	}
	for (noAddrIter iter; !iter.isEnd(); iter++) {
		(*iter)->close();
		delete *iter;
	}
}

//PUBLIC
void processGateways() {
	gatePoll.claim();

	while (running) {
		Event_Data data = gatePoll.wait();

		if (data.fd == 0)
			return;

		SOCKET sock = data.fd;
		UGateway * gate = (UGateway*)data.ptr;

		char test[1];
		if (recv(sock, test, 1, MSG_PEEK) == 0) //If there's no data when we were told there was, the socket closed
			delete gate;
		else
			gate->process();
	}
}

void processPeers() {
	peerPoll.claim();

	while (running) {
		Event_Data data = peerPoll.wait();

		if (data.fd == 0) {
			return;
		}

		SOCKET sock = data.fd;
		Peer * peer = (Peer*)data.ptr;

		char test[1];
		if (recv(sock, test, 1, MSG_PEEK) == 0) //If there's no data when we were told there was, the socket closed
			delete peer;
		else
			peer->process();
	}
}

//PUBLIC
void startup() {
	//Server construction
#ifdef _WIN32 //If compiled for Windows
	WSADATA socketConfig; //Construct WSA configuration destination
	WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock
#endif

	sockaddr_in gate = {
			AF_INET,
			htons(stoi(gatewayPort)),
			INADDR_ANY
	};
	sockaddr_in geds = {
			AF_INET,
			htons(stoi(peerPort)),
			INADDR_ANY
	};

	//Construct server sockets
	gateServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Construct gateway inbound socket
	gedsServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Construct GEDS P2P inbound socket

	//Bind servers to addresses
	if (::bind(gateServer, (sockaddr*)&gate, sizeof(sockaddr_in)) != 0 && errno == EADDRINUSE) { //Initialize gateway inbound socket
		error("Gateway port is in use");
		exit(-1);
	}
	if (::bind(gedsServer, (sockaddr*)&geds, sizeof(sockaddr_in)) != 0 && errno == EADDRINUSE) { //Initialize GEDS P2P inbound socket
		error("Peer port is in use");
		exit(-1);
	}

	//Activate servers
	listen(gateServer, SOMAXCONN); //Open gateway inbound socket
	listen(gedsServer, SOMAXCONN); //Open gateway inbound socket
	//Servers constructed and started

	//Add servers to poll
	serverPoll.add(gateServer);
	serverPoll.add(gedsServer);
}

//PUBLIC
void cleanup() {
#ifdef _WIN32
	closesocket(gedsServer);
	closesocket(gateServer);
#else
	close(gedsServer);
	close(gateServer);
#endif

	killConnections();

	gatePoll.update(); //Trigger worker threads to awaken to their death
	peerPoll.update();
}

//PUBLIC
void runServer() { //Listen for new connections
	serverPoll.claim();

	while (running) { //Dies on SIGINT
		Event_Data data = serverPoll.wait();

		if (data.fd == 0) {
			return;
		}

		SOCKET newSock = accept(data.fd, NULL, NULL);

#ifdef _WIN32
		serverPoll.remove(data.fd); //Work around winsock inheritance
		serverPoll.add(data.fd);
#endif

		try {
			if (data.fd == gateServer) {
				UGateway * gate = new UGateway(newSock);
				gatePoll.add(newSock, gate);

#ifdef _WIN32
				gatePoll.update();
#endif
			}
			else {
				Peer * peer = new Peer(newSock);
				peerPoll.add(newSock, peer);

#ifdef _WIN32
				peerPoll.update();
#endif
			}
		}
		catch (int e) {}
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
		setsockopt(*newSock, IPPROTO_TCP, TCP_SYNCNT, (void*)&opt, sizeof(opt)); //Correct excessive timeout period on Linux
#endif

		int result = connect(newSock, (sockaddr*)&addrFormat, iplen);

		if (result != 0) {
			warn("Failed to connect to " + ip.stringify() + " " + to_string(errno));
			continue;
		}

		Peer * newConn = new Peer(newSock, ip);

		newConn->transmit(ThisVersion.tostring());

		char death[3];
		timeval timeout = { 3, 0 };

		setsockopt(newSock, IPPROTO_TCP, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeval));
		int result2 = recv(newSock, death, 2, 0);

		if (result2 == -1) {
			delete newConn;
			error("Connection to " + ip.stringify() + " dropped during negotiation");
			continue;
		}

		if (death[0] == 0) {
			recv(newSock, death + 2, 1, 0);

			if (death[3] == 0) {
				warn("Peer " + ip.stringify() + " doesn't support " + ThisVersion.stringify());
				delete newConn;
				continue;
			}
			else if (death[3] == 1) {
				error("Peer " + ip.stringify() + " rejected this IP!");
				delete newConn;
				continue;
			}
		}

		newConn->state = 1;
		log("Connected to " + ip.stringify());

		peerPoll.add(newSock, newConn);
	}
}
