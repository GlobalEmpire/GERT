#pragma once
#include "../Gateway/RGateway.h"
#include <map>

typedef std::map<Address, RGateway*>::iterator routePtr;

class routeIter {
	routePtr ptr;
	public:
		bool isEnd();
		routeIter operator++(int);
		routeIter();
		routePtr operator->();
};

void killAssociated(Peer*);
