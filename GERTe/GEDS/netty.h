#pragma once
#include "Address.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;

#ifdef _WIN32
typedef unsigned long long SOCKET;
#else
typedef int SOCKET;
#else
typedef unsigned long long SOCKET;
#endif

void runServer();

void startup();
void cleanup();
void killConnections();
void processGateways();
void processPeers();
void buildWeb();