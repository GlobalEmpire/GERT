#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#endif

#include "Connection.h"
#include "netty.h"
#include "logging.h"

void Connection::error(char * err) {
	send(*(SOCKET*)sock, err, 3, 0);
	destroy(sock);
}

char * Connection::read(int num) {
	char * buf = new char[num+1];
	int len = recv(*(SOCKET*)this->sock, buf+1, num, 0);
	buf[0] = (char)len;
	return buf;
}

Connection::Connection(void * socket, std::string type) : sock(socket) {
	SOCKET * newSocket = (SOCKET*)sock;

#ifdef _WIN32
	ioctlsocket(*newSocket, FIONBIO, &nonZero);
#else
	int flags = fcntl(*newSocket, F_GETFL);
	fcntl(*newSocket, F_SETFL, flags | O_NONBLOCK);
#endif

	recv(*newSocket, vers, 2, 0);
	log(type + " using " + std::to_string(vers[0]) + "." + std::to_string(vers[1]));

	if (vers[0] != ThisVersion.major || vers[1] != ThisVersion.minor) { //Determine if major number is not supported
		char err[3] = { 0, 0, 0 };
		error(err);
		warn(type + "'s version wasn't supported!");
		throw 1;
	}
}
