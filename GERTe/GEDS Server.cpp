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

#define WIN32_LEAN_AND_MEAN //Reduce included windows API (removes incompatible Winsock v1)

#ifdef _WIN32 //If compiled for windows
#include <Windows.h> //Include windows API
#include <ws2tcpip.h> //Include windows TCP API
#include <iphlpapi.h> //Include windows IP helper API
#include <winsock2.h> //Include Winsock v2
typedef SOCKET SOCK; //Define SOCK type as Winsock SOCKET
typedef HMODULE lib; //Define lib type as Windows HMODULE
#else //If not compiled for windows (assumed linux)
#include <socket.h> //Load C++ standard socket API
#include <dlfcn.h> //Load C++ standard dynamic library API
#include <unistd.h>
#include <fcntl.h>
typedef int SOCK; //Define SOCK type as integer
typedef void* lib; //Define lib type as arbitrary pointer
#endif

typedef unsigned char UCHAR; //Creates UCHAR shortcut for Unsigned Character
typedef unsigned short ushort;
typedef map<GERTaddr, connection<gateway>> iter1;
typedef map<GERTaddr, connection<geds>> iter2;

#include <thread> //Include thread type
#include <string> //Include string type
#include <map> //Include map type
#include <signal.h> //Include signal processing API
#include <stdint.h>
#include <filesystem> //Include C++ standard filesystem (slightly modified by Windows to make it compatible)
using namespace std; //Default namespace to std
using namespace experimental::filesystem::v1; //Shorten C++ standard filesystem namespace

enum connType { //Define connection types of GATEWAY and GEDS
	GATEWAY,
	GEDS
};

enum status {
	NORMAL,
	NO_LIBS,
	LIB_LOAD_ERR
}

struct GERTaddr { //Define GERT address number
	unsigned short high; //First 3 bytes of address
	unsigned short low; //Last 4 bytes of address
};

struct GERTkey {
	char key[20]; //20 character long keystring
};

class version { //Define what a API version is
	public:
		lib* handle; //Store handle to loaded file
		UCHAR major, minor, patch; //Store three part version
		bool processGateway(gateway, string); //Store gateway processing function
		bool processGEDS(geds, string); //Store GEDS processing function
		void killGateway(gateway); //Safely destroy gateway connection
		void killGEDS(geds); //Safely destroy peer connection
};

struct gateway { //Define a connection
	UCHAR state; //Used for versions, defines what state the socket is in
	GERTaddr addr; //Used for versions, defines what the GERTaddr has been determined to be
};

struct geds {
	bool initialized;
	in_addr ip;
	ushort gateport;
	ushort gedsport;
};

template<class STATE>
class connection {
	SOCK * socket;
	public:
		connType type;
		STATE info;
		connection (SOCK*, connType, version);
		version api;
};

template<class STATE>
connection::connection(SOCK * sock, connType which, version vers) : type(which), socket(sock), api(vers) { //Create connection constructor
	if (which == GEDS) {
	} else if (which == GATEWAY) {
		info.state = 0;
		GERTaddr addr;
		addr.high = 0;
		addr.low = 0;
		info.addr = addr;
	}
}

map<UCHAR, version> knownVersions; //Create lookup for major version to API mapping
map<GERTaddr, connection<gateway>> gateways; //Create lookup for GERTaddr to connection mapping for gateway
map<in_addr, connection<geds>> peers; //Create lookup for IPv4 address to conneciton mapping for GEDS P2P
map<GERTaddr, connection<geds>> remoteRoute; //Create lookup for far gateways
map<GERTaddr, GERTkey> resolutions; //Create name resolution lookup table

int iplen = 14;

#ifdef _WIN32
u_long nonZero = 1;
#endif

volatile bool running = true; //SIGINT tracker
SOCK gateServer, gedsServer; //Define both server sockets
fd_set *gateTest, *gedsTest, *nullSet, allGates, allGEDS; //Define test sets and null set
TIMEVAL *nonBlock; //Define 0 duration timer

void registerVersion(version registee) {
	if (knownVersions.count(registee.major)) {
		version test = knownVersions[registee.major];
		if (test.minor < registee.minor) {
			knownVersions[registee.major] = registee;
		}
		else if (test.patch < registee.patch && test.minor == registee.patch){
			knownVersions[registee.major] = registee;
		}
		return;
	}
	knownVersions[registee.major] = registee;
}

void shutdownProceedure(int param) { //SIGINT handler function
	running = false; //Flip tracker to disable threads and trigger main thread's cleanup
};

lib loadLib(path libPath) { //Load gelib file
#ifdef _WIN32 //If compiled for Windows
	return LoadLibrary(libPath.c_str()); //Load using Windows API
#else //If not compiled for Windows
	return dlload(libPath.c_str(), RTLD_LAZY); //Load using C++ standard API
#endif
}

void* getValue(lib* handle, string value) { //Retrieve gelib value
#ifdef _WIN32 //If compiled for Windows
	return GetProcAddress(*handle, value.c_str()); //Retrieve using Windows API
#else //If not compiled for Windows
	return dlsym(handle, value.c_str()); //Retrieve using C++ standard API
#endif
}

void loadLibs() { //Load gelib files from api subfolder
	path libDir = "./apis/"; //Define relative path to subfolder
	directory_iterator iter(libDir), end; //Define file list and empty list
	while (iter != end) { //Continue until file list is equal to empty list
		directory_entry testFile = *iter; //Get file from list
		path testPath = testFile.path(); //Get path of file from list
		if (testPath.extension == "gelib") { //Test that file is a gelib file via file extension
			lib handle = loadLib(testPath); //Load gelib file
			version api; //Create new API version
			api.handle = &handle; //Set API handle
			api.major = (UCHAR)getValue(&handle, "major"); //Set API major version
			api.minor = (UCHAR)getValue(&handle, "minor"); //Set API minor version
			api.patch = (UCHAR)getValue(&handle, "patch"); //Set API patch version
			api.processGateway = getValue(&handle, "processGateway"); //Set API processGateway function
			api.processGEDS = getValue(&handle, "processGEDS"); //Set API processGEDS function
			api.killGateway = getValue(&handle, "killGateway");
			api.killGEDS = getValue(&handle, "killGEDS");
			registerVersion(api); //Register API and map API
		}
		iter++; //Move to next file
	}
}

void closeSock(SOCK target) { //Close a socket
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

connection getConnection(SOCK * sock) {
	return lookup[sock];
}

void process() {
	while (running) {
		sockiter iter = lookup.begin();
		sockiter end = lookup.end();
		for (sockiter iter = lookup.begin(); iter != lookup.end(); iter++) {
			char buf[256];
			SOCK sock = iter->first;
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

void initGERT(SOCK newSocket) {
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

void initGEDS(SOCK newSocket) {
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
			SOCK newSock = accept(gateServer, NULL, NULL);
			initGERT(newSock);
		}
		if (select(0, gedsTest, nullSet, nullSet, nonBlock) > 0) {//Tests GEDS P2P inbound socket
			SOCK newSocket = accept(gedsServer, NULL, NULL); //Accept connection from GEDS P2P inbound socket
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

void loadPeers() {

}

int main() {
	//Server construction
#ifdef _WIN32 //If compiled for Windows
	WSADATA socketConfig; //Construct WSA configuration destination
	WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock
#endif
	addrinfo *resultgate = NULL, *ptr = NULL, hints, *resultgeds; //Define some IP addressing
	ZeroMemory(&hints, sizeof(hints)); //Clear IP variables
	hints.ai_family = AF_INET; //Set to IPv4
	hints.ai_socktype = SOCK_STREAM; //Set to stream sockets
	hints.ai_protocol = IPPROTO_TCP; //Set to TCP
	hints.ai_flags = AI_PASSIVE; //Set to inbound socket
	getaddrinfo(NULL, GATEWAY_PORT, &hints, &resultgate); //Fill gateway inbound socket information
	getaddrinfo(NULL, PEER_PORT, &hints, &resultgeds); //Fille GEDS P2P inbound socket information
	gateServer = socket(resultgate->ai_family, resultgate->ai_socktype, resultgate->ai_protocol); //Construct gateway inbound socket
	gedsServer = socket(resultgeds->ai_family, resultgeds->ai_socktype, resultgeds->ai_protocol); //Construct GEDS P2P inbound socket
	bind(gateServer, resultgate->ai_addr, (int)resultgate->ai_addrlen); //Initialize gateway inbound socket
	bind(gedsServer, resultgeds->ai_addr, (int)resultgeds->ai_addrlen); //Initialize GEDS P2P inbound socket
	freeaddrinfo(resultgate); //Clear RAM
	freeaddrinfo(resultgeds); //Clear RAM
	listen(gateServer, SOMAXCONN); //Open gateway inbound socket
	listen(gedsServer, SOMAXCONN); //Open gateway inbound socket
	FD_ZERO(gateTest); //Clear gateway inbound test set
	FD_ZERO(gedsTest); //Clear GEDS P2P inbound test set
	FD_ZERO(nullSet); //Clear empty test set
	FD_SET(gateServer, &gateTest); //Assign inbound gateway socket to inbound gateway test set
	FD_SET(gedsServer, &gedsTest); //Assign GEDS P2P inbound socket to GEDS P2P test set
	nonBlock->tv_sec = 0; //Set 0 duration timer to 0 seconds
	nonBlock->tv_usec = 0; //Set 0 duration timer to 0 microseconds
	//Servers constructed and started

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
	return NORMAL;
}
