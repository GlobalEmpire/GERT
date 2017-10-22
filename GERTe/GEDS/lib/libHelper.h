#define __DYNLIB__
#include "../Key.h"
#include "../Gateway.h"
#include "../Peer.h"
#include "../GERTc.h"
#include "../logging.h"

extern "C" {
	extern bool sendToGateway(Address, string);
	extern bool isRemote(Address);
	extern void setRoute(Address, Peer*);
	extern void removeRoute(Address);
	extern void addResolution(Address, Key);
	extern void removeResolution(Address);
	extern void addPeer(IP, Ports);
	extern void removePeer(IP);
	extern void broadcast(string);
	extern string putAddr(Address);
	extern bool queryWeb(Address);
}
