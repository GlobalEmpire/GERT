#include "Connection.h"
#include "netty.h"
#include <sys/socket.h>

char * Connection::read(int num) {
	char * buf = new char[num+1];
	int len = recv(*(SOCKET*)this->sock, buf+1, num, 0);
	buf[0] = (char)len;
	return buf;
}
