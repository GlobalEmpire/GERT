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
#include "libLoad.h"
#include "peerManager.h"
#include "routeManager.h"
#include "gatewayManager.h"
#include "query.h"
#include "netty.h"
#include "logging.h"
#include <signal.h>
using namespace std;

SOCKET gateServer, gedsServer; //Define both server sockets
fd_set testSet; //Define test sets and null set

vector<int> gatefd;
vector<int> peerfd;

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
void processGateways() {
	pause();
	while (running) {
		pollfd set[gatefd.size()];
		for (int i = 0; i < gatefd.size(); i++) {
			set[i] = {
				gatefd[i],
				POLLIN,
				0
			};
		}
		poll(set, gatefd.size(), -1);
		for (int i = 0; i < gatefd.size(); i++) {
			if (set[i].revents & POLLIN) {
				Gateway(set[i].fd).process();
			} else if (set[i].revents & POLLHUP) {
				Gateway(set[i].fd).close();
			}
		}
		this_thread::yield();
	}
}

void processPeers() {
	pause();
	while (running) {
		pollfd set[peerfd.size()];
		for (int i = 0; i < peerfd.size(); i++) {
			set[i] = {
				peerfd[i],
				POLLIN,
				0
			};
		}
		poll(set, peerfd.size(), -1);
		for (int i = 0; i < peerfd.size(); i++) {
			if (set[i].revents & POLLIN) {
				Peer(set[i].fd).process();
			} else if (set[i].revents & POLLHUP) {
				Peer(set[i].fd).close();
			}
		}
		this_thread::yield();
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
	if (bind(gateServer, (sockaddr*)&gate, sizeof(sockaddr_in)) != 0 && errno == EADDRINUSE) { //Initialize gateway inbound socket
		error("Gateway port is in use");
		exit(-1);
	}
	if (bind(gedsServer, (sockaddr*)&geds, sizeof(sockaddr_in)) != 0 && errno == EADDRINUSE) { //Initialize GEDS P2P inbound socket
		error("Peer port is in use");
		exit(-1);
	}

	//Activate servers
	listen(gateServer, SOMAXCONN); //Open gateway inbound socket
	listen(gedsServer, SOMAXCONN); //Open gateway inbound socket
	//Servers constructed and started
}

//PUBLIC
void cleanup() {
	killConnections();
#ifdef _WIN32
	closesocket(gedsServer);
	closesocket(gateServer);
#else
	close(gedsServer);
	close(gateServer);
#endif
}

//PUBLIC
void runServer(void * gateways, void * peers) { //Listen for new connections
	while (running) { //Dies on SIGINT
		FD_ZERO(&testSet);
		FD_SET(gateServer, &testSet);
		FD_SET(gedsServer, &testSet);
		int result = select(FD_SETSIZE, &testSet, NULL, NULL, NULL);
		if (result == -1)
			break;
		if (FD_ISSET(gateServer, &testSet)) {
			SOCKET * newSock = new SOCKET;
			*newSock = accept(gateServer, NULL, NULL);
			try {
				new Gateway(newSock);
				gatefd.push_back(*newSock);
				pthread_kill(*(thread::native_handle_type*)gateways, SIGINT);
			} catch(int e) {}
		}
		if (FD_ISSET(gedsServer, &testSet)) {//Tests GEDS P2P inbound socket
			SOCKET * newSocket = new SOCKET; //Accept connection from GEDS P2P inbound socket
			*newSocket = accept(gedsServer, NULL, NULL);
			Peer * peer;
			try {
				peer = new Peer(newSocket);
				peerfd.push_back(*newSocket);
				pthread_kill(*(thread::native_handle_type*)peers, SIGINT);
			} catch(int e) {}
		}
		this_thread::yield(); //Release CPU
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
		char verc[3] = {vers.major, vers.minor, vers.patch};
		send(*newSock, verc, (ULONG)3, 0);
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
		Peer* newConn = new Peer((void*)newSock, best, &known);
		newConn->state = 1;
		log("Connected to " + ip.stringify());
	}
}

extern "C" {
	string putAddr(Address addr) {
		const unsigned char* chars = addr.getAddr();
		return string{chars[0], chars[1], chars[2]};
	}
}
