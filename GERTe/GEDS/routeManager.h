#pragma once
#include "Peer.h"
#include "Address.h"
#include <map>
typedef std::map<Address, Peer*>::iterator routePtr;

class routeIter {
	routePtr ptr;
	public:
		bool isEnd();
		routeIter operator++(int);
		routeIter();
		routePtr operator->();
};

void killAssociated(Peer*);
void setRoute(Address, Peer*);
void removeRoute(Address);
bool remoteSend(Address, std::string);
bool isRemote(Address);
