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
typedef int SOCK; //Define SOCK type as integer
typedef void* lib; //Define lib type as arbitrary pointer
#endif

typedef unsigned char UCHAR; //Creates UCHAR shortcut for Unsigned Character

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

};

class version { //Define what a API version is
	public:
		lib* handle; //Store handle to loaded file
		UCHAR major, minor, patch; //Store three part version
		bool processGateway(connection*, string); //Store gateway processing function
		bool processGEDS(connection*, string); //Store GEDS processing function
};

struct connection { //Define a connection
	SOCK * socket; //Store reference to socket
	UCHAR state; //Used for versions, defines what state the socket is in
	connType type; //What type of connection this is (shouldn't be needed)
	GERTaddr addr; //Used for versions, defines what the GERTaddr has been determined to be
	in_addr ip; //Used for versions, defines the connection's remote IPv4 address
	version api; //API version used for this connection
};

map<int, version> knownVersions; //Create lookup for major version to API mapping
map<GERTaddr, connection> gateways; //Create lookup for GERTaddr to connection mapping for gateway
map<in_addr, connection> peers; //Create lookup for IPv4 address to conneciton mapping for GEDS P2P
map<SOCK, connection> lookup; //Create lookup for socket handle mapping for all sockets
map<GERTaddr, connection> remoteRoute; //Create lookup for far gateways

volatile bool running = true; //SIGINT tracker
SOCK gateServer, gedsServer; //Define both server sockets
fd_set *gateTest, *gedsTest, *nullSet; //Define test sets and null set
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

void socketError() { //Socket failure handler
	printf("SOCKET FAILURE"); //Output error to console
#ifdef _WIN32 //If compiled for windows
	WSACleanup(); //Trigger winsock cleanup
#endif
	abort(); //Immediately exit program
}

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
	else //If connection is a GEDS server
		peers.erase(target.ip); //Remove it from the GEDS map
	closeSock(*target.socket); //Close the socket
}

void sendTo(GERTaddr addr, string data) {
	connection target = gateways[addr];
	send(*target.socket, data.c_str(), data.length, NULL);
}

void assign(connection requestee, GERTaddr requested) {
	requestee.addr = requested;
	gateways[requested] = requestee;
}

void addResolution(GERTaddr addr, GERTkey key) {

}

void listen() { //Listen for new connections
	while (running) { //Dies on SIGINT
		int result1 = select(0, gateTest, nullSet, nullSet, nonBlock); //Tests gateway inbound socket
		int result2 = select(0, gedsTest, nullSet, nullSet, nonBlock); //Tests GEDS P2P inbound socket
		if (result1) { //Initialize connection from gateway
			SOCK newSocket = accept(gateServer, NULL, NULL); //Accept connetion from gateway inbound socket
			char buf[3];
			recv(newSocket, buf, 3, NULL); //Read first 3 bytes, the version data requested by gateway
			unsigned char major = buf[0]; //Major version number
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
				api.processGateway(&newConnection, ""); //Initialize socket with empty packet using processGateway
			}
		}
		if (result2) { //Initialize GEDS P2P connection
			SOCK newSocket = accept(gedsServer, NULL, NULL); //Accept connection from GEDS P2P inbound socket
		}
		this_thread::yield(); //Release CPU
	}
}

int main() {
#ifdef _WIN32 //If compiled for Windows
	WSADATA socketConfig; //Construct WSA configuration destination
	WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock
#endif
	struct addrinfo *resultgate = NULL, *ptr = NULL, hints, *resultgeds; //Define some IP addressing
	ZeroMemory(&hints, sizeof(hints)); //Clear IP variables
	hints.ai_family = AF_INET; //Set to IPv4
	hints.ai_socktype = SOCK_STREAM; //Set to stream sockets
	hints.ai_protocol = IPPROTO_TCP; //Set to TCP
	hints.ai_flags = AI_PASSIVE; //Set to inbound socket
	getaddrinfo(NULL, GATEWAY_PORT, &hints, &resultgate); //Fill gateway inbound socket information
	getaddrinfo(NULL, PEER_PORT, &hints, &resultgeds); //Fille GEDS P2P inbound socket information
	gateServer = socket(resultgate->ai_family, resultgate->ai_socktype, resultgate->ai_protocol); //Construct gateway inbound socket
	gedsServer = socket(resultgeds->ai_family, resultgeds->ai_socktype, resultgeds->ai_protocol); //Construct GEDS P2P inbound socket
	if (gateServer == INVALID_SOCKET || gedsServer == INVALID_SOCKET) { //Double check sockets constructed
		freeaddrinfo(resultgate); //Clear RAM
		freeaddrinfo(resultgeds); //Clear RAM
		socketError(); //Print error and abort
	}
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
	thread peerHandler(socketry.peer);
	while (running) {
		this_thread::yield();
	}
	listener.join();
	peerHandler.join();
	return 0;
}