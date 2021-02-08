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
#include "../Util/logging.h"
#include "../Util/Versioning.h"
#include "../Util/Error.h"

std::string Connection::read(int num) const {
	char * buf = new char[num];
	int len = recv(this->sock, buf, num, 0);
    std::string data{ buf, (size_t)len };
    delete[] buf;
	return data;
}

Connection::Connection(SOCKET socket, const std::string& type) : sock(socket) {
#ifdef _WIN32
#define PTR char*
#else
#define PTR void*
#endif

#ifdef WIN32
	WSAEventSelect(socket, nullptr, 0); //Clears all events associated with the new socket
	u_long nonblock = 0;
	ioctlsocket(socket, FIONBIO, &nonblock); //Ensure socket is in blocking mode
#endif

#ifndef _WIN32
	timeval opt = { 1, 0 };
#else
	int opt = 2000;
#endif

	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (PTR)&opt, sizeof(opt));

#undef PTR

	int result = recv(socket, (char*)vers, 2, 0);

	if (result != 2) {
		socketError("Error reading version information: ");
		return;
	}

	log(type + " using v" + std::to_string(vers[0]) + "." + std::to_string(vers[1]));

	if (vers[0] != ThisVersion.major) { //Determine if major number is not supported
		write("\0\0\0");
		warn(type + "'s version wasn't supported!");
		return;
	}
	else if (vers[1] > ThisVersion.minor)
		vers[1] = ThisVersion.minor;

	valid = true;
}

Connection::Connection(SOCKET socket) : sock(socket) {}	

Connection::~Connection() {
    if (valid)
        close();
}

void Connection::write(const std::string& data) const {
    send(sock, data.c_str(), (ULONG)data.length(), 0);
}

void Connection::close() const {
#ifdef WIN32
    closesocket(sock);
#else
    ::close(sock);
#endif

    valid = false;
}
