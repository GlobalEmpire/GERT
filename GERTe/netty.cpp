#define WIN32_LEAN_AND_MEAN

#ifdef _WIN32 //If compiled for windows
#include <Windows.h> //Include windows API
#include <ws2tcpip.h> //Include windows TCP API
#include <iphlpapi.h> //Include windows IP helper API
#include <winsock2.h> //Include Winsock v2
#else //If not compiled for windows (assumed linux)
#include <socket.h> //Load C++ standard socket API
#include <unistd.h>
#include <fcntl.h>
typedef int SOCKET; //Define SOCK type as integer
#endif
#include "netDefs.h"
#include "config.h"
#include <thread>
#include <map>
#include <forward_list>
#include "keyMngr.h"
using namespace std;

//WARNING: DOES NOT HANDLE NETWORK PROCESSING INTERNALLY!!!

#ifndef GATEWAY_PORT
#error Gateway port not defined. Either the configuration file is missing or corrupted.
#endif
#ifndef PEER_PORT
#error Peer port not defined. The configuration file is corrupted.
#endif

map<in_addr, knownPeer> peerList;
map<in_addr, peer> peers;
map<GERTaddr, gateway> gateways;
map<GERTaddr, peer> remoteRoutes;

forward_list<SOCKET> sockets;

SOCKET gateServer, gedsServer; //Define both server sockets
fd_set *gateTest, *gedsTest, *nullSet, allGates, allGEDS; //Define test sets and null set
TIMEVAL nonBlock = { 0, 0 }; //Define 0 duration timer

#ifdef _WIN32
u_long nonZero = 1;
#endif

int iplen = 14;

extern volatile bool running;

void closeSock(SOCKET target) { //Close a socket
#ifdef _WIN32 //If compiled for Windows
	closesocket(target); //Close socket using WinSock API
#else //If not compiled for Windows
	close(target); //Close socket using C++ standard API
#endif
}


void killConnections() {
	for (sockiter iter = lookup.begin(); iter != lookup.end(); iter++) {
		connection conn = iter->second;
		if (conn.type == GATEWAY)
			conn.api.killGateway(conn);
		else if (conn.type == GEDS)
			conn.api.killGEDS(conn);
	}
}

//PUBLIC
void startup() {
	//Server construction
#ifdef _WIN32 //If compiled for Windows
	WSADATA socketConfig; //Construct WSA configuration destination
	WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock
#endif

	//Define some needed variables
	addrinfo *resultgate = NULL, *ptr = NULL, hints, *resultgeds; //Define some IP addressing
	ZeroMemory(&hints, sizeof(hints)); //Clear IP variables
	hints.ai_family = AF_INET; //Set to IPv4
	hints.ai_socktype = SOCK_STREAM; //Set to stream sockets
	hints.ai_protocol = IPPROTO_TCP; //Set to TCP
	hints.ai_flags = AI_PASSIVE; //Set to inbound socket

	//Request internal addressing and port
	getaddrinfo(NULL, GATEWAY_PORT, &hints, &resultgate); //Fill gateway inbound socket information
	getaddrinfo(NULL, PEER_PORT, &hints, &resultgeds); //Fille GEDS P2P inbound socket information

	//Construct server sockets
	SOCKET gateServer = socket(resultgate->ai_family, resultgate->ai_socktype, resultgate->ai_protocol); //Construct gateway inbound socket
	SOCKET gedsServer = socket(resultgeds->ai_family, resultgeds->ai_socktype, resultgeds->ai_protocol); //Construct GEDS P2P inbound socket

	//Bind servers to addresses
	bind(gateServer, resultgate->ai_addr, (int)resultgate->ai_addrlen); //Initialize gateway inbound socket
	bind(gedsServer, resultgeds->ai_addr, (int)resultgeds->ai_addrlen); //Initialize GEDS P2P inbound socket

	//Destroy some used variables
	freeaddrinfo(resultgate); //Clear RAM
	freeaddrinfo(resultgeds); //Clear RAM

	//Activate servers
	listen(gateServer, SOMAXCONN); //Open gateway inbound socket
	listen(gedsServer, SOMAXCONN); //Open gateway inbound socket
	//Servers consructed and started

	//Construct some socket sets for testing serves
	FD_ZERO(gateTest); //Clear gateway inbound test set
	FD_ZERO(gedsTest); //Clear GEDS P2P inbound test set
	FD_ZERO(nullSet); //Clear empty test set
	FD_SET(gateServer, &gateTest); //Assign inbound gateway socket to inbound gateway test set
	FD_SET(gedsServer, &gedsTest); //Assign GEDS P2P inbound socket to GEDS P2P test set
}

//PUBLIC
void shutdown() {
	//KillConnections()
	closeSock(gateServer);
	closeSock(gedsServer);
}

void initGate(SOCKET newSocket) {
#ifdef _WIN32
	ioctlsocket(newSocket, FIONBIO, &nonZero);
#else
	fcntl(newSocket, F_SETFL, O_NONBLOCK);
#endif
	char buf[3];
	recv(newSocket, buf, 3, NULL); //Read first 3 bytes, the version data requested by gateway
	UCHAR major = buf[0]; //Major version number
	version* api = getVersion(major);
	if (api == nullptr) {
		char error[3] = { 0, 0, 0 };
		send(newSocket, error, 3, NULL); //Notify client we cannot serve this version
		closeSock(newSocket); //Close the socket
	}
	else { //Major version found
		gateway newConnection(&newSocket, *api);
		sockets.push_front(newSocket);
		newConnection.process("");
	}
}

void initPeer(SOCKET newSocket) {
#ifdef _WIN32
	ioctlsocket(newSocket, FIONBIO, &nonZero);
#else
	fcntl(newSocket, F_SETFL, O_NONBLOCK);
#endif
	char buf[3];
	recv(newSocket, buf, 3, NULL);
	UCHAR major = buf[0]; //Major version number
	version* api = getVersion(major); //Find API version
	if (api == nullptr) { //Determine if major number is not supported
		char error[3] = { 0, 0, 0 };
		send(newSocket, error, 3, NULL); //Notify client we cannot serve this version
		closeSock(newSocket); //Close the socket
	}
	else { //Major version found
		sockaddr* remotename;
		getpeername(newSocket, remotename, &iplen);
		sockaddr_in remoteip = *(sockaddr_in*)remotename;
		if (peers.count(remoteip.sin_addr) == 0) {
			char error[3] = { 0, 0, 1 };
			send(newSocket, error, 3, NULL);
		}
		peer newConnection(&newSocket, *api, remoteip.sin_addr); //Create new connection
		peers[remoteip.sin_addr] = newConnection;
		newConnection.process("");
	}
}

//PUBLIC
void runServer() { //Listen for new connections
	while (running) { //Dies on SIGINT
		if (select(0, gateTest, nullSet, nullSet, &nonBlock) > 0) {
			SOCKET newSock = accept(gateServer, NULL, NULL);
			initGate(newSock);
		}
		if (select(0, gedsTest, nullSet, nullSet, &nonBlock) > 0) {//Tests GEDS P2P inbound socket
			SOCKET newSocket = accept(gedsServer, NULL, NULL); //Accept connection from GEDS P2P inbound socket
			initPeer(newSocket);
		}
		this_thread::yield(); //Release CPU
	}
}

bool assign(gateway requestee, GERTaddr requested, GERTkey key) {
	if (checkKey(requested, key)) {
		requestee.addr = requested;
		gateways[requested] = requestee;
		return true;
	}
	return false;
}

//PUBLIC
void addPeer(in_addr addr, portComplex ports) {
	peerList[addr] = { addr, ports };
}

//PUBLIC
void removePeer(in_addr addr) {
	peerList.erase(addr);
}

void setRoute(GERTaddr target, peer route) {
	remoteRoutes[target] = route;
}

void removeRoute(GERTaddr target) {
	remoteRoutes.erase(target);
}