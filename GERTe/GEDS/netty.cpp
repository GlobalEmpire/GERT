#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h> //Load C++ standard socket API
#endif
#include <sys/types.h>
#include <thread>
#include "libLoad.h"
#include "peerManager.h"
#include "routeManager.h"
#include "gatewayManager.h"
#include "query.h"
#include "netty.h"
#include "logging.h"
#include "Poll.h"
#include "API.h"
using namespace std;

SOCKET gateServer, gedsServer; //Define both server sockets

Poll gatePoll;
Poll peerPoll;
Poll serverPoll;

extern volatile bool running;
extern char * gatewayPort;
extern char * peerPort;
extern char * LOCAL_IP;
extern vector<Gateway*> noAddrList;

void destroy(SOCKET * target) { //Close a socket
#ifdef _WIN32 //If compiled for Windows
	closesocket(*target); //Close socket using WinSock API
#else //If not compiled for Windows
	close(*target); //Close socket using C++ standard API
#endif
	delete target;
}

void destroy(void * target) {
	destroy((SOCKET*) target);
}

void killConnections() {
	for (gatewayIter iter; !iter.isEnd(); iter++) {
		killGateway(*iter);
	}
	for (peerIter iter; !iter.isEnd(); iter++) {
		killGEDS(*iter);
	}
	for (noAddrIter iter; !iter.isEnd(); iter++) {
		killGateway(*iter);
	}
}

//PUBLIC
void processGateways() {
	while (running) {
		Event_Data data = gatePoll.wait();

		SOCKET sock = data.fd;
		Gateway * gate = (Gateway*)data.ptr;

		char test[1];
		if (recv(sock, test, 1, MSG_PEEK) == 0) //If there's no data when we were told there was, the socket closed
			killGateway(gate);
		else
			processGateway(gate);
	}
}

void processPeers() {
	while (running) {
		Event_Data data = gatePoll.wait();

		SOCKET sock = data.fd;
		Peer * peer = (Peer*)data.ptr;

		char test[1];
		if (recv(sock, test, 1, MSG_PEEK) == 0) //If there's no data when we were told there was, the socket closed
			killGEDS(peer);
		else
			processGEDS(peer);
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
}

//PUBLIC
void runServer() { //Listen for new connections
	while (running) { //Dies on SIGINT
		Event_Data data = serverPoll.wait();
		SOCKET * newSock = new SOCKET;
		*newSock = accept(data.fd, NULL, NULL);
		try {
			if (data.fd == gateServer) {
				Gateway * gate = new Gateway(newSock);
				gatePoll.add(*newSock, gate);
			}
			else {
				Peer * peer = new Peer(newSock);
				peerPoll.add(*newSock, peer);
			}
		}
		catch (int e) {}
	}
}

void buildWeb() {
	Version* best = getVersion(highestVersion());
	Versioning vers = best->vers;
	for (knownIter iter; !iter.isEnd(); iter++) {
		KnownPeer known = *iter;
		IP ip = known.addr;
		Ports ports = known.ports;
		if (ports.peer == 0) {
			debug("Skipping peer " + ip.stringify() + " because it's outbound only.");
			continue;
		} else if (ip.stringify() == LOCAL_IP)
			continue;
		SOCKET * newSock = new SOCKET;
		*newSock = socket(AF_INET, SOCK_STREAM, 0);
		in_addr remoteIP = ip.addr;
		sockaddr_in addrFormat;
		addrFormat.sin_addr = remoteIP;
		addrFormat.sin_port = ports.peer;
		addrFormat.sin_family = AF_INET;
		int result = connect(*newSock, (sockaddr*)&addrFormat, iplen);
		if (result != 0) {
			warn("Failed to connect to " + ip.stringify() + " " + to_string(errno));
			continue;
		}
		unsigned char verc[3] = {vers.major, vers.minor, vers.patch};
		send(*newSock, (const char *)verc, (ULONG)3, 0);
		char death[3];
		pollfd pollReq = {*newSock, POLLIN};
#ifdef _WIN32
		int result2 = WSAPoll(&pollReq, 1, 5);
#else
		int result2 = poll(&pollReq, 1, 5);
#endif
		if (result2 < 1) {
			destroy(newSock);
			error("Connection to " + ip.stringify() + " dropped during negotiation");
		}
		recv(*newSock, death, 3, 0);
		if (death[0] == 0) {
			warn("Peer " + ip.stringify() + " doesn't support " + vers.stringify());
			destroy(newSock);
			continue;
		}

		Peer* newConn = new Peer((void*)newSock, best, &known);
		newConn->state = 1;
		log("Connected to " + ip.stringify());
	}
}