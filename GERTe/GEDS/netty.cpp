#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <sys/socket.h> //Load C++ standard socket API
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <poll.h>
#include <thread>
#include <cstring>
#include "libLoad.h"
#include "peerManager.h"
#include "routeManager.h"
#include "gatewayManager.h"
#include "netty.h"
using namespace std;

typedef timeval TIMEVAL;

SOCKET gateServer, gedsServer; //Define both server sockets
fd_set gateTest, gedsTest, nullSet; //Define test sets and null set
TIMEVAL nonBlock = { 0, 0 }; //Define 0 duration timer

extern volatile bool running;
extern char * gatewayPort;
extern char * peerPort;
extern char * LOCAL_IP;

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
		(*iter)->kill();
	}
	for (peerIter iter; !iter.isEnd(); iter++) {
		(*iter)->kill();
	}
	for (noAddrIter iter; !iter.isEnd(); iter++) {
		(*iter)->kill();
	}
}

//PUBLIC
void process() {
	while (running) {
		for (gatewayIter iter; !iter.isEnd(); iter++) {
			if (*iter == nullptr)
				break;
			char buf[256];
			gateway* conn = *iter;
			int result = recv(*(SOCKET*)conn->sock, buf, 256, 0);
			if (result == 0)
				continue;
			conn->process(buf);
		}
		for (peerIter iter; !iter.isEnd(); iter++) {
			if (*iter == nullptr)
				break;
			char buf[256];
			peer* conn = *iter;
			if (conn == nullptr || (SOCKET*)(conn->sock) == nullptr) {
				error("WHAT THE HECK. IT'S NULL!");
				abort();
			}
			int result = recv(*(SOCKET*)conn->sock, buf, 256, 0);
			if (result == 0)
				continue;
			conn->process(buf);
		}
		for (noAddrIter iter; !iter.isEnd(); iter++) {
			if (*iter == nullptr)
				break;
			char buf[256];
			gateway* conn = *iter;
			int result = recv(*(SOCKET*)conn->sock, buf, 256, 0);
			if (result == 0)
				continue;
			conn->process(buf);
		}
		this_thread::yield();
	}
	killConnections();
}

//PUBLIC
void startup() {
	//Server construction
#ifdef _WIN32 //If compiled for Windows
	WSADATA socketConfig; //Construct WSA configuration destination
	WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock
#endif

	//Define some needed variables
	addrinfo *resultgate = NULL, hints, *resultgeds; //Define some IP addressing
	memset(&hints, 0, sizeof(hints)); //Clear IP variables
	hints.ai_family = AF_INET; //Set to IPv4
	hints.ai_socktype = SOCK_STREAM; //Set to stream sockets
	hints.ai_protocol = IPPROTO_TCP; //Set to TCP
	hints.ai_flags = AI_PASSIVE; //Set to inbound socket

	//Request internal addressing and port
	getaddrinfo(NULL, gatewayPort, &hints, &resultgate); //Fill gateway inbound socket information
	getaddrinfo(NULL, peerPort, &hints, &resultgeds); //Fill GEDS P2P inbound socket information

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
	//Servers constructed and started

	//Construct some socket sets for testing serves
	FD_ZERO(&gateTest); //Clear gateway inbound test set
	FD_ZERO(&gedsTest); //Clear GEDS P2P inbound test set
	FD_ZERO(&nullSet); //Clear empty test set
	FD_SET(gateServer, &gateTest); //Assign inbound gateway socket to inbound gateway test set
	FD_SET(gedsServer, &gedsTest); //Assign GEDS P2P inbound socket to GEDS P2P test set
}

//PUBLIC
void shutdown() {
	//KillConnections()
#ifdef _WIN32
	closesocket(gedsServer);
	closesocket(gateServer);
#else
	close(gedsServer);
	close(gateServer);
#endif
}

//PUBLIC
void runServer() { //Listen for new connections
	while (running) { //Dies on SIGINT
		if (select(0, &gateTest, &nullSet, &nullSet, &nonBlock) > 0) {
			SOCKET * newSock = new SOCKET;
			*newSock = accept(gateServer, NULL, NULL);
			initGate((void*)newSock);
		}
		if (select(0, &gedsTest, &nullSet, &nullSet, &nonBlock) > 0) {//Tests GEDS P2P inbound socket
			SOCKET * newSocket = new SOCKET; //Accept connection from GEDS P2P inbound socket
			*newSocket = accept(gedsServer, NULL, NULL);
			initPeer((void*)newSocket);
		}
		this_thread::yield(); //Release CPU
	}
}

//PUBLIC
void sendTo(gateway* target, string data) {
	send(*(SOCKET*)target->sock, data.c_str(), (ULONG)data.length(), 0);
}

//PUBLIC
void sendTo(peer* target, string data) {
	send(*(SOCKET*)target->sock, data.c_str(), (ULONG)data.length(), 0);
}

void buildWeb() {
	version* best = getVersion(highestVersion());
	versioning vers = best->vers;
	for (knownIter iter; !iter.isEnd(); iter++) {
		ipAddr ip = (*iter).addr;
		knownPeer known = *iter;
		portComplex ports = known.ports;
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
		send(*newSock, (to_string(vers.major) + to_string(vers.minor) + to_string(vers.patch)).c_str(), (ULONG)3, 0);
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
#ifdef _WIN32
		ioctlsocket(*newSock, FIONBIO, &nonZero);
#else
		fcntl(*newSock, F_SETFL, O_NONBLOCK);
#endif
		peer* newConn = new peer((void*)newSock, best, remoteIP);
		newConn->state = 1;
		_raw_peer(remoteIP, newConn);
		log("Connected to " + ip.stringify());
	}
}

GERTaddr getAddr(string data) {
	USHORT * ptr = (USHORT*)data.c_str();
	return GERTaddr{ntohs(*ptr), ntohs(*ptr++)};
}

string putAddr(GERTaddr addr) {
	USHORT high = htons(addr.high);
	USHORT low = htons(addr.low);
	USHORT * ptr1 = &high;
	USHORT * ptr2 = &low;
	return string{*(char*)ptr1, *(char*)ptr1++, *(char*)ptr2, *(char*)ptr2++};
}

portComplex makePorts(string data) {
	USHORT * ptr = (USHORT*)data.c_str();
	return portComplex{ntohs(*ptr), ntohs(*ptr++)};
}
