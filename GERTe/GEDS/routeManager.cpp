#include "netDefs.h"
#include "gatewayManager.h"
#include "routeManager.h"
#include "netty.h"
#include <map>

typedef map<GERTaddr, peer*>::iterator routePtr;

map<GERTaddr, peer*> routes;

bool routeIter::isEnd() { return ptr == routes.end(); }
routeIter routeIter::operator++ (int a) { return (ptr++, *this); }
routeIter::routeIter() : ptr(routes.begin()) {};
routePtr routeIter::operator-> () { return ptr; }

void killAssociated(peer* target) {
	for (routeIter iter; !iter.isEnd(); iter++) {
		if (iter->second == target) {
			gateway* toDie = getGate(iter->first);
			toDie->kill();
			routes.erase(iter->first);
		}
	}
}

void setRoute(GERTaddr target, peer* route) {
	routes[target] = route;
	log("Received routing information for " + target.stringify());
}

void removeRoute(GERTaddr target) {
	routes.erase(target);
	log("Lost routing information for " + target.stringify());
}

bool isRemote(GERTaddr target) {
	return routes.count(target) > 0;
}

bool remoteSend(GERTaddr target, string data) {
	if (routes.count(target) == 0)
		return false;
	sendTo(routes[target], data);
	return true;
}
