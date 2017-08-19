#ifndef __NETTY__
#define __NETTY__
#include "keyMngr.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef int SOCKET;

const unsigned int iplen = 16;

extern "C" {
	void destroy(void*);
	void startup();
	void cleanup();
	void runServer();
	void killConnections();
	void sendByGateway(Gateway*, string);
	void sendByPeer(peer*, string);
	void process();
	void buildWeb();
	string putAddr(Address);
}
#endif
