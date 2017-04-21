/* 
	This is the primary code for the GEDS server.
	This file implements platform independent definitions for internet sockets.
	Supported platforms are currently Linux(/Unix) and Windows
	To modify the default port change the numbers in the GATEWAY_PORT and PEER_PORT definitions.
	
	This code is designed to function with other code from
	https://github.com/GlobalEmpire/GERT/
	
	This code falls under the license located at
	https://github.com/GlobalEmpire/GERT/blob/master/License.md
 */

#define GATEWAY_PORT "43780" //Define inbound gateway port
#define PEER_PORT "59474" //Define inbound GEDS P2P port

typedef unsigned char UCHAR; //Creates UCHAR shortcut for Unsigned Character
typedef unsigned short ushort;
typedef map<GERTaddr, connection<gateway>>::iterator iter1;
typedef map<GERTaddr, connection<geds>>::iterator iter2;

#include <thread> //Include thread type
#include <string> //Include string type
#include <map> //Include map type
#include <signal.h> //Include signal processing API
#include <stdint.h>
#include "netDefs"
using namespace std; //Default namespace to std

enum status {
	NORMAL,
	NO_LIBS,
	LIB_LOAD_ERR
}

struct GERTkey {
	char key[20]; //20 character long keystring
};

map<GERTaddr, connection<gateway>> gateways; //Create lookup for GERTaddr to connection mapping for gateway
map<in_addr, connection<geds>> peers; //Create lookup for IPv4 address to conneciton mapping for GEDS P2P
map<GERTaddr, connection<geds>> remoteRoute; //Create lookup for far gateways
map<GERTaddr, GERTkey> resolutions; //Create name resolution lookup table

int iplen = 14;

#ifdef _WIN32
u_long nonZero = 1;
#endif

volatile bool running = true; //SIGINT tracker
SOCKET gateServer, gedsServer; //Define both server sockets
fd_set *gateTest, *gedsTest, *nullSet, allGates, allGEDS; //Define test sets and null set
TIMEVAL *nonBlock; //Define 0 duration timer

void shutdownProceedure(int param) { //SIGINT handler function
	running = false; //Flip tracker to disable threads and trigger main thread's cleanup
};

void closeSock(SOCKET target) { //Close a socket
#ifdef _WIN32 //If compiled for Windows
	closesocket(target); //Close socket using WinSock API
#else //If not compiled for Windows
	close(target); //Close socket using C++ standard API
#endif
}

//MARKED FOR UPDATING
void closeConnection(connection target) { //Close a full connection
	lookup.erase(*target.socket); //Remove connection from universal map
	if (target.type == GATEWAY) //If connection is a gateway
		gateway gate = *(gateway*)target.ptr;
	else {//If connection is a GEDS server
		
		peers[target.ip] = empty;
	}
	closeSock(*target.socket); //Close the socket
}

void sendTo(GERTaddr addr, string data) {
	gateway target = gateways[addr];
	send(*target.socket, data.c_str(), data.length, NULL);
}

void sendTo(connection conn, string data) {
	send(*conn.socket, data.c_str(), data.length, NULL);
}

bool assign(gateway requestee, GERTaddr requested, GERTkey key) {
	if (resolutions[requested].key == key.key) {
		requestee.addr = requested;
		gateways[requested] = requestee;
		return true;
	}
	return false;
}

void addResolution(GERTaddr addr, GERTkey key) {
	resolutions[addr] = key;
}

void removeResolution(GERTaddr addr) {
	resolutions.erase(addr);
}

void addPeer(char* ip, ushort gatePort, ushort gedsPort) {
	in_addr peer = { ip[0], ip[1], ip[2], ip[3] };
	geds peerConn;
	peerConn.ip = peer;
	peerConn.gateport = gatePort;
	peerConn.gedsport = gedsPort;
	peers[peer] = peerConn;
}

void removePeer(char* ip) {
	in_addr peer = { ip[0], ip[1], ip[2], ip[3] };
	geds target = peers[peer];
	target.api.killGEDS(target);
	peers.erase(peer);
}

void setRoute(GERTaddr target, geds remote) {
	remoteRoute[target] = remote;
}

void removeRoute(GERTaddr target) {
	remoteRoute.erase(target);
}

connection getConnection(SOCKET * sock) {
	return lookup[sock];
}

void process() {
	while (running) {
		sockiter iter = lookup.begin();
		sockiter end = lookup.end();
		for (sockiter iter = lookup.begin(); iter != lookup.end(); iter++) {
			char buf[256];
			SOCKET sock = iter->first;
			connection conn = iter->second;
			int result = recv(sock, buf, 256, NULL);
			if (result = 0)
				continue;
			else if (conn.type == GATEWAY)
				conn.api.processGateway(*(gateway*)conn.ptr, buf);
			else if (conn.type == GEDS)
				conn.api.processGEDS(*(geds*)conn.ptr, buf);
		}
		this_thread::yield();
	}
}

void initGERT(SOCKET newSocket) {
#ifdef _WIN32
	ioctlsocket(newSocket, FIONBIO, &nonZero);
#else
	fcntl(newSocket, F_SETFL, O_NONBLOCK);
#endif
	char buf[3];
	recv(newSocket, buf, 3, NULL); //Read first 3 bytes, the version data requested by gateway
	UCHAR major = buf[0]; //Major version number
	if (knownVersions.count(major) == 0) { //Determine if major number is not supported
		char error[3] = { 0, 0, 0 };
		send(newSocket, error, 3, NULL); //Notify client we cannot serve this version
		closeSock(newSocket); //Close the socket
	}
	else { //Major version found
		version api = knownVersions[major]; //Find API version
		connection<gateway> newConnection{ newSocket, GATEWAY, api }; //Create new connection
		//FIX THIS
		lookup[newSocket] = newConnection; //Create socket to connection map entry
		api.processGateway(newConnection, ""); //Initialize socket with empty packet using processGateway
	}
}

void initGEDS(SOCKET newSocket) {
#ifdef _WIN32
	ioctlsocket(newSocket, FIONBIO, &nonZero);
#else
	fcntl(newSocket, F_SETFL, O_NONBLOCK);
#endif
	char buf[3];
	recv(newSocket, buf, 3, NULL);
	UCHAR major = buf[0]; //Major version number
	if (knownVersions.count(major) == 0) { //Determine if major number is not supported
		char error[3] = { 0, 0, 0 };
		send(newSocket, error, 3, NULL); //Notify client we cannot serve this version
		closeSock(newSocket); //Close the socket
	}
	else { //Major version found
		version api = knownVersions[major]; //Find API version
		connection<geds> newConnection{newSocket, GEDS, api}; //Create new connection
		sockaddr* remotename;
		getpeername(newSocket, remotename, &iplen);
		sockaddr_in remoteip = *(sockaddr_in*)remotename;
		if (peers.count(remoteip.sin_addr) == 0) {
			char error[3] = { 0, 0, 1 };
			send(newSocket, error, 3, NULL);
		}
		geds peer = peers[remoteip.sin_addr];
		peer.api = api;
		peer.socket = &newSocket;
		newConnection.ptr = &peer;
		lookup[newSocket] = newConnection;
		api.processGEDS(peer, ""); //Initialize socket with empty packet using processGateway
	}
}

void listen() { //Listen for new connections
	while (running) { //Dies on SIGINT
		if (select(0, gateTest, nullSet, nullSet, nonBlock) > 0) {
			SOCKET newSock = accept(gateServer, NULL, NULL);
			initGERT(newSock);
		}
		if (select(0, gedsTest, nullSet, nullSet, nonBlock) > 0) {//Tests GEDS P2P inbound socket
			SOCKET newSocket = accept(gedsServer, NULL, NULL); //Accept connection from GEDS P2P inbound socket
			initGEDS(newSocket);
		}
		this_thread::yield(); //Release CPU
	}
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

void loadResolutions() {

}

string readableIP(in_addr ip) {
	UCHAR * seg = &ip;
	string high = to_string(seg[0]) + "." + to_string(seg[1]);
	string low = to_string(seg[2]) + "." + to_string(seg[3]);
	return high + "." + low;
}

void loadPeers() {
	FILE peerFile = fopen("peers.geds", "rb");
	while (true) {
		in_addr ip;
		fread(&ip, 4, 1, peerFile); //Why must I choose between 1, 4 and 4, 1? Or 2, 2?
		portComplex ports;
		fread(&ports, 2, 2, peerFile);
		cout << "Imported peer " << readableIP(ip) << ":" << ports.gatePort << ":" << ports.gedsPort << "\n";
		geds peer;
		peer.ip = ip;
		peer.ports = ports;
		initalized = false;
	}
}

int main() {
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

	//Set nonblock's timer duration to 0
	nonBlock->tv_sec = 0; //Set 0 duration timer to 0 seconds
	nonBlock->tv_usec = 0; //Set 0 duration timer to 0 microseconds

	loadLibs(); //Load gelib files

	//System processing
	signal(SIGINT, &shutdownProceedure); //Hook SIGINT with custom handler
	thread processor(process);
	listen(); //Server fully active

	//Shutdown and Cleanup sequence
	processor.join(); //Cleanup processor (wait for it to die)
	killConnections(); //Kill any remaining connections

	//Cleanup servers
#ifdef _WIN32
	shutdown(gateServer, SD_BOTH);
	shutdown(gedsServer, SD_BOTH);
	WSACleanup();
#else
	close(gateServer);
	close(gedsServer);
#endif

	//Return with exit state "NORMAL" (0)
	return NORMAL;
}
