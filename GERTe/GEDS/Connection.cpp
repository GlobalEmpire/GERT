#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#endif

#include "Connection.h"
#include "netty.h"

char * Connection::read(int num) {
	char * buf = new char[num+1];
	int len = recv(*(SOCKET*)this->sock, buf+1, num, 0);
	buf[0] = (char)len;
	return buf;
}
