#include "Connection.h"
#include "netty.h"
#include <sys/socket.h>

char * Connection::read(int num) {
	char * buf = new char[num];
	recv(*(SOCKET*)this->sock, buf, num, 0);
	return buf;
}
