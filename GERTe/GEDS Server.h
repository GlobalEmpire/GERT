/*
 * This is the primary code header for the GEDS server.
 * This file's primary purpose is providing needed symbols to API files.
 * All needed components are included within this header.
 * Include this header to compile APIs with symbols.
 */

#include "netty.h" //Also loads netDefs which loads libLoad

typedef unsigned char UCHAR;

struct GERTkey {
	char key[20];
};

void startup() {};
void shutdown() {};
void runServer() {};
void killConnections() {};
void process() {};

struct knownPeer {};

enum libErrors{};
void loadLibs() {};
void getVersion(UCHAR) {};
