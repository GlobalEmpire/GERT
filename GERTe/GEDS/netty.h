#pragma once
#include "Address.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;

#ifdef _WIN32
typedef unsigned long long SOCKET;
#else
typedef int SOCKET;
#endif

void runServer();

void startup(unsigned short, unsigned short);
void cleanup();
void killConnections();
void processGateways();
void processPeers();
void buildWeb(char *);