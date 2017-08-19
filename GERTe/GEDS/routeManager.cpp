#include "routeManager.h"
#include "gatewayManager.h"
#include "netty.h"

map<Address, peer*> routes;

bool routeIter::isEnd() { return ptr == routes.end(); }
routeIter routeIter::operator++ (int a) { return (ptr++, *this); }
routeIter::routeIter() : ptr(routes.begin()) {};
routePtr routeIter::operator-> () { return ptr; }

void killAssociated(peer* target) {
	for (routeIter iter; !iter.isEnd(); iter++) {
		if (iter->second == target) {
			Gateway* toDie = getGate(iter->first);
			toDie->kill();
			routes.erase(iter->first);
		}
	}
}

void setRoute(Address target, peer* route) {
	routes[target] = route;
	log("Received routing information for " + target.stringify());
}

void removeRoute(Address target) {
	routes.erase(target);
	log("Lost routing information for " + target.stringify());
}

bool isRemote(Address target) {
	return routes.count(target) > 0;
}

bool remoteSend(Address target, string data) {
	if (routes.count(target) == 0)
		return false;
	sendByPeer(routes[target], data);
	return true;
}
