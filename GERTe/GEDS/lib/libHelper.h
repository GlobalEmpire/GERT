#define __DYNLIB__
#include "../keyDef.h"
#include "../netDefs.h"

extern bool sendToGateway(GERTaddr, string);
extern void sendByGateway(gateway*, string);
extern void sendByPeer(peer*, string);
extern bool assign(gateway*, GERTaddr, GERTkey);
extern bool isRemote(GERTaddr);
extern bool isLocal(GERTaddr);
extern void closeGateway(gateway*);
extern void closePeer(peer*);
extern void setRoute(GERTaddr, peer*);
extern void removeRoute(GERTaddr);
extern void addResolution(GERTaddr, GERTkey);
extern void removeResolution(GERTaddr);
extern void addPeer(ipAddr, portComplex);
extern void removePeer(ipAddr);
extern void broadcast(string);
extern GERTaddr getAddr(string);
extern string putAddr(GERTaddr);
extern portComplex makePorts(string);
extern bool queryWeb(GERTaddr);
