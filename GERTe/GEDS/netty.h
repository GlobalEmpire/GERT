#pragma once
#include "Address.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;

void runServer();

void startup();
void cleanup();
void killConnections();
void processGateways();
void processPeers();
void buildWeb();