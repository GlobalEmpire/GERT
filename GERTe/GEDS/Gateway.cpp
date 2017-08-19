#include "Gateway.h"
#include "libLoad.h"
#include "netty.h"
#include <sys/socket.h>
#include <fcntl.h>

typedef int SOCKET;

Gateway::Gateway(void* sock) : connection(sock) {
	SOCKET * newSocket = (SOCKET*)sock; //Convert socket to correct type
	char buf[3]; //Create a buffer for the version data
	recv(*newSocket, buf, 3, 0); //Read first 3 bytes, the version data requested by gateway
	log((string)"Gateway using v" + to_string(buf[0]) + "." + to_string(buf[1]) + "." + to_string(buf[2])); //Notify user of connection and version
	UCHAR major = buf[0]; //Major version number
	api = getVersion(major); //Get the protocol library for the major version
#ifdef _WIN32 //If we're compiled in Windows
	ioctlsocket(*newSocket, FIONBIO, &nonZero); //Make socket non-blocking
#else //If we're compiled in *nix
	fcntl(*newSocket, F_SETFL, O_NONBLOCK); //Make socket non-blocking
#endif
	if (api == nullptr) { //If the protocol library doesn't exist
		char error[3] = { 0, 0, 0 }; //Construct the error code
		send(*newSocket, error, 3, 0); //Notify client we cannot serve this version
		destroy(newSocket); //Close the socket
		throw;
	} else {
		this->process(""); //Process empty data (Protocol Library Gateway Initialization)
	}
}
