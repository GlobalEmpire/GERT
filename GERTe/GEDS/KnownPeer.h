#include "netDefs.h"

class KnownPeer {
public:
	ipAddr addr;
	portComplex ports;
	KnownPeer(ipAddr target, portComplex bindings) : addr(target), ports(bindings) {};
	KnownPeer() : addr(0L), ports() {};
};

KnownPeer * getKnown(sockaddr_in);
