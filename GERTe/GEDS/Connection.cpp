#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#endif

#include "Connection.h"
#include "netty.h"
#include "logging.h"
#include "Versioning.h"
#include "Error.h"

void Connection::error(char * err) {
	send(sock, err, 3, 0);
}

void Connection::setopts() {
#ifndef _WIN32
#define PTR void*
	timeval opt = { 1, 0 };
#else
#define PTR char*
	int opt = 2000;
#endif

	// Ensure the socket is in blocking mode
#ifdef WIN32
	WSAEventSelect(sock, NULL, 0); //Clears all events associated with the new socket
	u_long nonblock = 1;
	int resulterr = ioctlsocket(sock, FIONBIO, &nonblock);
#else
	fcntl(sock, F_SETFL, O_NONBLOCK);
#endif

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (PTR)& opt, sizeof(opt));

	//Set the socket connection maintenance parameters
	bool keepalive = true;
	char idle = 10;
	char delay = 2;
	char count = 5;

	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (PTR)& keepalive, sizeof(keepalive));
	setsockopt(sock, SOCK_STREAM, TCP_KEEPIDLE, (PTR)& idle, sizeof(idle));
	setsockopt(sock, SOCK_STREAM, TCP_KEEPINTVL, (PTR)& delay, sizeof(delay));
	setsockopt(sock, SOCK_STREAM, TCP_KEEPCNT, (PTR)& count, sizeof(count));
#undef PTR
}

Connection::Connection(SOCKET socket, std::string type) {
	sock = socket;

	setopts();
}

Connection::Connection() {}

Connection::~Connection() {
#ifdef WIN32
	closesocket(sock);
#else
	::close(sock);
#endif
}

bool Connection::negotiate(std::string type) {
	start:
	if (vers[0] == 0)
		if (consume(2)) {
			vers[0] = buf[0];
			vers[1] = buf[1];
			clean();

			log(type + " using v" + std::to_string(vers[0]) + "." + std::to_string(vers[1]));

			if (vers[0] != ThisVersion.major) { //Determine if major number is not supported
				char err[3] = { 0, 0, 0 };
				error(err);
				warn(type + "'s version wasn't supported!");
				delete this;
				return false;
			}

			if (vers[1] > ThisVersion.minor)
				vers[1] = ThisVersion.minor;

			goto start;
		}
	else {
		if (vers[1] == 0) {
			if (consume(1)) {
				clean();
				return true;
			}

			return false;
		}

		return true;
	}

	return false;
}
