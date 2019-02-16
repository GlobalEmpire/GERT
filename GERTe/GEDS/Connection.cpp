#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "Connection.h"
#include "netty.h"
#include "logging.h"
#include "Versioning.h"

void Connection::error(char * err) {
	send(*(SOCKET*)sock, err, 3, 0);
	delete this;
}

char * Connection::read(int num) {
	char * buf = new char[num+1];
	int len = recv(*(SOCKET*)this->sock, buf+1, num, 0);
	buf[0] = (char)len;
	return buf;
}

Connection::Connection(void * socket, std::string type) : sock(socket) {
	SOCKET * newSocket = (SOCKET*)sock;

	timeval opt = { 1, 0 };

#ifdef _WIN32
#define PTR char*
#else
#define PTR void*
#endif

	setsockopt(*newSocket, SOL_SOCKET, SO_RCVTIMEO, (PTR)&opt, sizeof(opt));

#undef PTR

	int result = recv(*newSocket, vers, 2, 0);

	if (result != 2) {
		::error("New connection failed to send sufficient version information: " + std::to_string(result) + " " + std::to_string(errno));
		throw 1;
	}

	log(type + " using " + std::to_string(vers[0]) + "." + std::to_string(vers[1]));

	if (vers[0] != ThisVersion.major) { //Determine if major number is not supported
		char err[3] = { 0, 0, 0 };
		error(err);
		warn(type + "'s version wasn't supported!");
		throw 1;
	}

	//If version is pre-1.1.0 then read an extra byte for compatibility
	if (vers[1] == 0) {
		char spare;
		recv(*newSocket, &spare, 1, 0);
	}
}

Connection::Connection(void * socket) : sock(socket) {}	

Connection::~Connection() {
#ifdef WIN32
	closesocket(*(SOCKET*)sock);
#else
	close(*(SOCKET*)sock);
#endif
}
