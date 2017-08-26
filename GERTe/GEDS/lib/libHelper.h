#define __DYNLIB__
#include "../netDefs.h"
#include "../Key.h"
#include "../Gateway.h"
#include "../Peer.h"

extern "C" {
	extern bool sendToGateway(Address, string);
	extern bool isRemote(Address);
	extern bool isLocal(Address);
	extern void setRoute(Address, Peer*);
	extern void removeRoute(Address);
	extern void addResolution(Address, Key);
	extern void removeResolution(Address);
	extern void addPeer(ipAddr, portComplex);
	extern void removePeer(ipAddr);
	extern void broadcast(string);
	extern string putAddr(Address);
	extern portComplex makePorts(string);
	extern bool queryWeb(Address);
}
