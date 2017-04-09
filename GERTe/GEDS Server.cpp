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
typedef map<SOCK, connection>::iterator sockiter;

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

struct GERTaddr { //Define GERT address number
	unsigned short high;
	unsigned short low;
};

struct GERTkey {
	char key[20];
};

class version { //Define what a API version is
	public:
		lib* handle; //Store handle to loaded file
		UCHAR major, minor, patch; //Store three part version
		bool processGateway(connection, string); //Store gateway processing function
		bool processGEDS(connection, string); //Store GEDS processing function
		void killGateway(connection);
		void killGEDS(connection);
};

struct connection { //Define a connection
	SOCK * socket; //Store reference to socket
	UCHAR state; //Used for versions, defines what state the socket is in
	connType type; //What type of connection this is (shouldn't be needed)
	GERTaddr addr; //Used for versions, defines what the GERTaddr has been determined to be
	in_addr ip; //Used for versions, defines the connection's remote IPv4 address
	version api; //API version used for this connection
};

map<UCHAR, version> knownVersions; //Create lookup for major version to API mapping
map<GERTaddr, connection> gateways; //Create lookup for GERTaddr to connection mapping for gateway
map<in_addr, connection> peers; //Create lookup for IPv4 address to conneciton mapping for GEDS P2P
map<SOCK, connection> lookup; //Create lookup for socket handle mapping for all sockets
map<GERTaddr, connection> remoteRoute; //Create lookup for far gateways
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

void closeConnection(connection target) { //Close a full connection
	lookup.erase(*target.socket); //Remove connection from universal map
	if (target.type == GATEWAY) //If connection is a gateway
		gateways.erase(target.addr); //Remove it from gateway map
	else {//If connection is a GEDS server
		connection empty;
		empty.ip = target.ip;
		empty.type = GEDS;
		peers[target.ip] = empty;
	}
	closeSock(*target.socket); //Close the socket
}

void sendTo(GERTaddr addr, string data) {
	connection target = gateways[addr];
	send(*target.socket, data.c_str(), data.length, NULL);
}

void sendTo(connection conn, string data) {
	send(*conn.socket, data.c_str(), data.length, NULL);
}

bool assign(connection requestee, GERTaddr requested, GERTkey key) {
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

void addPeer(char* ip) {
	in_addr peer = { ip[0], ip[1], ip[2], ip[3] };
	connection peerConn;
	peerConn.ip = peer;
	peerConn.type = GEDS;
	peers[peer] = peerConn;
}

void removePeer(char* ip) {
	in_addr peer = { ip[0], ip[1], ip[2], ip[3] };
	peers.erase(peer);
}

void setRoute(GERTaddr target, connection remote) {
	remoteRoute[target] = remote;
}

void removeRoute(GERTaddr target) {
	remoteRoute.erase(target);
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
				conn.api.processGateway(conn, buf);
			else if (conn.type == GEDS)
				conn.api.processGEDS(conn, buf);
		}
		this_thread::yield();
	}
}

void listen() { //Listen for new connections
	while (running) { //Dies on SIGINT
		int result1 = select(0, gateTest, nullSet, nullSet, nonBlock); //Tests gateway inbound socket
		int result2 = select(0, gedsTest, nullSet, nullSet, nonBlock); //Tests GEDS P2P inbound socket
		if (result1) { //Initialize connection from gateway
			SOCK newSocket = accept(gateServer, NULL, NULL); //Accept connetion from gateway inbound socket
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
			} else { //Major version found
				version api = knownVersions[major]; //Find API version
				connection newConnection; //Create new connection
				newConnection.socket = &newSocket; //Set connection socket handle
				newConnection.type = GATEWAY; //Set connection type
				newConnection.api = api;
				lookup[newSocket] = newConnection; //Create socket to connection map entry
				api.processGateway(newConnection, ""); //Initialize socket with empty packet using processGateway
			}
		}
		if (result2) { //Initialize GEDS P2P connection
			SOCK newSocket = accept(gedsServer, NULL, NULL); //Accept connection from GEDS P2P inbound socket
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
				connection newConnection; //Create new connection
				newConnection.socket = &newSocket; //Set connection socket handle
				newConnection.type = GEDS; //Set connection type
				newConnection.api = api;
				lookup[newSocket] = newConnection; //Create socket to connection map entry
				sockaddr* remotename;
				getpeername(newSocket, remotename, &iplen);
				sockaddr_in remoteip = *(sockaddr_in*)remotename;
				peers[remoteip.sin_addr] = newConnection;
				api.processGateway(newConnection, ""); //Initialize socket with empty packet using processGateway
			}
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
	loadLibs(); //Load gelib files
	signal(SIGINT, &shutdownProceedure); //Hook SIGINT with custom handler
	thread processor(process);
	listen();
	processor.join();
	killConnections();
	return 0;
}