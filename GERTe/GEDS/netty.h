#pragma once
#include "Address.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef int SOCKET;

constexpr unsigned int iplen = 16;

extern "C" {
	void destroy(void*);
	void startup();
	void cleanup();
	void runServer();
	void killConnections();
	void process();
	void buildWeb();
	string putAddr(Address);
}
