#ifndef __NETTY__
#define __NETTY__
#include "netDefs.h"
#include "keyMngr.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef int SOCKET;

const unsigned int iplen = 16;

void destroy(void*);
void startup();
void shutdown();
void runServer();
void killConnections();
void sendTo(gateway*, string);
void sendTo(peer*, string);
void process();
void buildWeb();
#endif
