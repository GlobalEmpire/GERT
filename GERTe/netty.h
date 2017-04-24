#include "netDefs.h"

typedef unsigned char UCHAR;

void startup();
void shutdown();
void runServer();
void addPeer(in_addr, portComplex);
void removePeer(in_addr);
void setRoute(GERTaddr, peer);
void removeRoute(GERTaddr);
void killConnections();
bool sendTo(GERTaddr, string);
void sendTo(in_addr, string);
void sendTo(gateway, string);
void sendTo(peer, string);
bool isRemote(GERTaddr);
void process();
void closeTarget(gateway);
void closeTarget(peer);
