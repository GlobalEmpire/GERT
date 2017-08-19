#define __DYNLIB__
#include "../netDefs.h"
#include "../Key.h"
#include "../Gateway.h"

extern "C" {
	extern bool sendToGateway(Address, string);
	extern void sendByGateway(Gateway*, string);
	extern void sendByPeer(peer*, string);
	extern bool assign(Gateway*, Address, Key);
	extern bool isRemote(Address);
	extern bool isLocal(Address);
	extern void closeGateway(Gateway*);
	extern void closePeer(peer*);
	extern void setRoute(Address, peer*);
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
