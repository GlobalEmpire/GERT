#define WIN32_LEAN_AND_MEAN

#ifdef _WIN32 //If compiled for windows
#include <Windows.h> //Include windows API
#include <ws2tcpip.h> //Include windows TCP API
#include <iphlpapi.h> //Include windows IP helper API
#include <winsock2.h> //Include Winsock v2
#else //If not compiled for windows (assumed linux)
#include <sys/socket.h> //Load C++ standard socket API
#include <netinet/ip.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
typedef int SOCKET; //Define SOCK type as integer
typedef timeval TIMEVAL;
#endif
#include "netDefs.h"
#include "config.h"
#include <thread>
#include <map>
#include <forward_list>
#include <cstring>
#include "keyMngr.h"
#include "libLoad.h"
#include "logging.h"
using namespace std;

typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef unsigned char UCHAR;
typedef map<GERTaddr, gateway*>::iterator gateIter;
typedef map<GERTaddr, peer*>::iterator remoteIter;
typedef forward_list<gateway*>::iterator noAddrIter;
typedef map<ipAddr, peer*>::iterator peerIter;
typedef map<ipAddr, knownPeer>::iterator knownIter;


//WARNING: DOES NOT HANDLE NETWORK PROCESSING INTERNALLY!!!

#ifndef GATEWAY_PORT
#error Gateway port not defined. Either the configuration file is missing or corrupted.
#endif
#ifndef PEER_PORT
#error Peer port not defined. The configuration file is corrupted.
#endif

map<ipAddr, knownPeer> peerList;
map<ipAddr, peer*> peers;
map<GERTaddr, gateway*> gateways;
map<GERTaddr, peer*> remoteRoutes;

forward_list<gateway*> noAddr;

SOCKET gateServer, gedsServer; //Define both server sockets
fd_set gateTest, gedsTest, nullSet; //Define test sets and null set
TIMEVAL nonBlock = { 0, 0 }; //Define 0 duration timer

#ifdef _WIN32
u_long nonZero = 1;
#endif

UINT iplen = 16;

extern volatile bool running;

void closeSock(SOCKET target) { //Close a socket
#ifdef _WIN32 //If compiled for Windows
	closesocket(target); //Close socket using WinSock API
#else //If not compiled for Windows
	close(target); //Close socket using C++ standard API
#endif
}


void killConnections() {
	for (gateIter iter = gateways.begin(); iter != gateways.end(); iter++) {
		iter->second->kill();
	}
	for (peerIter iter = peers.begin(); iter != peers.end(); iter++) {
		iter->second->kill();
	}
}

//PUBLIC
void closeTarget(gateway* target) { //Close a full connection
	gateways.erase(target->addr); //Remove connection from universal map
	closeSock(*(SOCKET*)target->sock); //Close the socket
	log("Disassociation from " + target->addr.stringify());
}

//PUBLIC
void closeTarget(peer* target) {
	for (remoteIter iter = remoteRoutes.begin(); iter != remoteRoutes.end(); iter++) {
		if (iter->second->addr == target->addr)
			remoteRoutes.erase(iter->first);
	}
	peerIter iter = peers.find(target->addr);
	peers.erase(iter);
	closeSock(*(SOCKET*)target->sock);
	log("Peer " + target->addr.stringify() + " disconnected");
}

//PUBLIC
void process() {
	while (running) {
		for (gateIter iter = gateways.begin(); iter != gateways.end(); iter++) {
			if (iter->second == nullptr)
				break;
			char buf[256];
			gateway* conn = iter->second;
			int result = recv(*(SOCKET*)conn->sock, buf, 256, 0);
			if (result == 0)
				continue;
			conn->process(buf);
		}
		for (peerIter iter = peers.begin(); iter != peers.end(); iter++) {
			if (iter->second == nullptr)
				break;
			char buf[256];
			peer* conn = iter->second;
			if (conn == nullptr || (SOCKET*)(conn->sock) == nullptr) {
				error("WHAT THE HECK. IT'S NULL!");
				abort();
			}
			int result = recv(*(SOCKET*)conn->sock, buf, 256, 0);
			if (result == 0)
				continue;
			conn->process(buf);
		}
		for (noAddrIter iter = noAddr.begin(); iter != noAddr.end(); iter++) {
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
	getaddrinfo(NULL, GATEWAY_PORT, &hints, &resultgate); //Fill gateway inbound socket information
	getaddrinfo(NULL, PEER_PORT, &hints, &resultgeds); //Fill GEDS P2P inbound socket information

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
	recv(newSocket, buf, 3, 0); //Read first 3 bytes, the version data requested by gateway
	log((string)"Gateway using " + buf[0] + "." + buf[1] + "." + buf[2]);
	UCHAR major = buf[0]; //Major version number
	version* api = getVersion(major);
	if (api == nullptr) {
		char error[3] = { 0, 0, 0 };
		send(newSocket, error, 3, 0); //Notify client we cannot serve this version
		closeSock(newSocket); //Close the socket
	}
	else { //Major version found
		gateway * newConnection = new gateway(&newSocket, api);
		noAddr.push_front(newConnection);
		newConnection->process("");
	}
}

void initPeer(SOCKET newSocket) {
#ifdef _WIN32
	ioctlsocket(newSocket, FIONBIO, &nonZero);
#else
	fcntl(newSocket, F_SETFL, O_NONBLOCK);
#endif
	char buf[3];
	recv(newSocket, buf, 3, 0);
	log((string)"GEDS using " + buf[0] + "." + buf[1] + "." + buf[2]);
	UCHAR major = buf[0]; //Major version number
	version* api = getVersion(major); //Find API version
	if (api == nullptr) { //Determine if major number is not supported
		char error[3] = { 0, 0, 0 };
		send(newSocket, error, 3, 0); //Notify client we cannot serve this version
		closeSock(newSocket); //Close the socket
	}
	else { //Major version found
		sockaddr* remotename;
		getpeername(newSocket, remotename, &iplen);
		sockaddr_in remoteip = *(sockaddr_in*)remotename;
		if (peerList.count(remoteip.sin_addr) == 0) {
			char error[3] = { 0, 0, 1 }; //STATUS ERROR NOT_AUTHORIZED
			send(newSocket, error, 3, 0);
		}
		peer* newConnection = new peer(&newSocket, api, remoteip.sin_addr); //Create new connection
		peers[remoteip.sin_addr] = newConnection;
		log("Peer connected from " + newConnection->addr.stringify());
		newConnection->process("");
	}
}

//PUBLIC
void runServer() { //Listen for new connections
	while (running) { //Dies on SIGINT
		if (select(0, &gateTest, &nullSet, &nullSet, &nonBlock) > 0) {
			SOCKET * newSock = new SOCKET;
			*newSock = accept(gateServer, NULL, NULL);
			initGate(*newSock);
		}
		if (select(0, &gedsTest, &nullSet, &nullSet, &nonBlock) > 0) {//Tests GEDS P2P inbound socket
			SOCKET * newSocket = new SOCKET; //Accept connection from GEDS P2P inbound socket
			*newSocket = accept(gedsServer, NULL, NULL);
			initPeer(*newSocket);
		}
		this_thread::yield(); //Release CPU
	}
}

bool assign(gateway* requestee, GERTaddr requested, GERTkey key) {
	if (checkKey(requested, key)) {
		requestee->addr = requested;
		gateways[requested] = requestee;
		noAddrIter last = noAddr.begin();
		if (*last == requestee) {
			noAddr.pop_front();
			return true;
		}
		for (noAddrIter iter = noAddr.begin(); iter != noAddr.end(); iter++) {
			if (*iter == requestee) {
				noAddr.erase_after(last);
				break;
			}
			last++;
		}
		log("Association from " + requested.stringify());
		return true;
	}
	return false;
}

//PUBLIC
void addPeer(ipAddr addr, portComplex ports) {
	peerList[addr] = knownPeer(addr, ports);
	if (running)
		log("New peer " + addr.stringify());
}

//PUBLIC
void removePeer(ipAddr addr) {
	map<ipAddr, knownPeer>::iterator iter = peerList.find(addr);
	peerList.erase(iter);
	log("Removed peer " + addr.stringify());
}

//PUBLIC
void setRoute(GERTaddr target, peer* route) {
	remoteRoutes[target] = route;
	log("Received routing information for " + target.stringify());
}

//PUBLIC
void removeRoute(GERTaddr target) {
	remoteRoutes.erase(target);
	log("Lost routing information for " + target.stringify());
}

//PUBLIC COMPAT
bool isRemote(GERTaddr target) {
	return (bool)remoteRoutes.count(target);
}

//PUBLIC
bool sendTo(GERTaddr addr, string data) {
	if (gateways.count(addr) != 0)
		send(*(SOCKET*)gateways[addr]->sock, data.c_str(), (ULONG)data.length(), 0);
	else if (remoteRoutes.count(addr) != 0)
		send(*(SOCKET*)remoteRoutes[addr]->sock, data.c_str(), (ULONG)data.length(), 0); //ENSURE ROUTE AND DATA ARE SAME COMMAND CODE
	else
		return false;
	return true;
}

//PUBLIC
void sendTo(ipAddr addr, string data) {
	send(*(SOCKET*)((peer*)peers[addr])->sock, data.c_str(), (ULONG)data.length(), 0);
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
	in_addr local;
	inet_aton(LOCAL_IP, &local);
	for (knownIter iter = peerList.begin(); iter != peerList.end(); iter++) {
		if (iter->second.ports.peer == 0)
			continue;
		SOCKET * newSock = new SOCKET;
		*newSock = socket(AF_INET, SOCK_STREAM, 0);
		in_addr remoteIP = iter->second.addr.addr;
		if (remoteIP.s_addr == local.s_addr)
			continue;
		in_port_t peerPort = iter->second.ports.peer;
		sockaddr_in addrFormat;
		addrFormat.sin_addr = remoteIP;
		addrFormat.sin_port = peerPort;
		addrFormat.sin_family = AF_INET;
		int result = connect(*newSock, (sockaddr*)&addrFormat, iplen);
		if (result != 0) {
			warn("Failed to connect to " + iter->second.addr.stringify() + " " + to_string(errno));
			continue;
		}
		send(*newSock, (to_string(vers.major) + to_string(vers.minor) + to_string(vers.patch)).c_str(), (ULONG)3, 0);
		char death[3];
		recv(*newSock, death, 3, 0);
		ipAddr ip = remoteIP;
		if (death[0] == 0) {
			warn("Peer " + ip.stringify() + " doesn't support " + vers.stringify());
			closeSock(*newSock);
			continue;
		}
#ifdef _WIN32
		ioctlsocket(*newSock, FIONBIO, &nonZero);
#else
		fcntl(*newSock, F_SETFL, O_NONBLOCK);
#endif
		peer* newConn = new peer((void*)newSock, best, remoteIP);
		newConn->state = 1;
		peers[ip] = newConn;
		log("Connected to " + ip.stringify());
	}
}

