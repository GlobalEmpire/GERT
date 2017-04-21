#define WIN32_LEAN_AND_MEAN //Reduce included windows API (removes incompatible Winsock v1)
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
#include "netDefs"

//WARNING: DOES NOT HANDLE NETWORK PROCESSING INTERNALLY!!!

#ifndef GATEWAY_PORT
#error Gateway port not defined.
#endif
#ifndef PEER_PORT
#error Peer port not defined.
#endif

#ifdef _WIN32
WSAConfig

void startup() {

}
