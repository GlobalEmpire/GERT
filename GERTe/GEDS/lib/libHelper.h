#define __DYNLIB__
#include "../keyDef.h"
#include "../netDefs.h"

extern bool sendTo(GERTaddr, string);
extern void sendTo(gateway*, string);
extern void sendTo(peer*, string);
extern bool assign(gateway*, GERTaddr, GERTkey);
extern bool isRemote(GERTaddr);
extern void closeTarget(gateway*);
extern void closeTarget(peer*);
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
