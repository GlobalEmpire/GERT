#include "IP.h"
#include "Ports.h"

class KnownPeer {
public:
	IP addr;
	Ports ports;
	KnownPeer(IP target, Ports bindings) : addr(target), ports(bindings) {};
	KnownPeer() : addr(0L), ports() {};
};

KnownPeer * getKnown(sockaddr_in);
