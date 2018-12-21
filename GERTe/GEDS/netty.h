#pragma once
#include "Address.h"
#include "Versioning.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;

#ifndef _WIN32
typedef int SOCKET;
#else
typedef unsigned long long SOCKET;
#endif

constexpr unsigned int iplen = 16;

constexpr Versioning vers{ 1, 0, 1 };

void runServer();

void destroy(void*);
void startup();
void cleanup();
void killConnections();
void processGateways();
void processPeers();
void buildWeb();