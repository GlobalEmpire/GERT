#ifndef __NETTY__
#define __NETTY__
#include "netDefs.h"
#include "keyMngr.h"

typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;

void startup();
void shutdown();
void runServer();
void addPeer(ipAddr, portComplex);
void removePeer(ipAddr);
void setRoute(GERTaddr, peer*);
void removeRoute(GERTaddr);
void killConnections();
bool sendTo(GERTaddr, string);
void sendTo(ipAddr, string);
void sendTo(gateway*, string);
void sendTo(peer*, string);
bool isRemote(GERTaddr);
void process();
void closeTarget(gateway*);
void closeTarget(peer*);
bool assign(gateway*, GERTaddr, GERTkey);
void buildWeb();
#endif
