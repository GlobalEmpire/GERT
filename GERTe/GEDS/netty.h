#pragma once
#include "Address.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;

#ifndef _WIN32
typedef int SOCKET;
#endif

constexpr unsigned int iplen = 16;

void runServer(void*, void*);

extern "C" {
	void destroy(void*);
	void startup();
	void cleanup();
	void killConnections();
	void processGateways();
	void processPeers();
	void buildWeb();
}
