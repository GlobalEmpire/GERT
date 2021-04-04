#pragma once

#include <vector>
#include "../Gateway/Address.h"
#include "../Peer/CommandConnection.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;

#ifdef _WIN32
typedef unsigned long long SOCKET;
#else
typedef int SOCKET;
#endif

SOCKET createSocket(uint32_t, unsigned short);

void runServer(std::vector<CommandConnection*>);

void startup();
void cleanup();