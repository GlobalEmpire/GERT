#include "routeManager.h"
#include "gatewayManager.h"
#include "netty.h"
#include "logging.h"

std::map<Address, Peer*> routes;

bool routeIter::isEnd() { return (ptr == routes.end()) || (ptr == ++routes.end()); }
routeIter routeIter::operator++ (int a) { return (ptr++, *this); }
routeIter::routeIter() : ptr(routes.begin()) {};
routePtr routeIter::operator-> () { return ptr; }

void killAssociated(Peer* target) {
	for (routeIter iter; !iter.isEnd(); iter++) {
		if (iter->second == target) {
			Gateway* toDie = Gateway::lookup(iter->first);
			toDie->close();
			routes.erase(iter->first);
		}
	}
}

void setRoute(Address target, Peer* route) {
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

bool remoteSend(Address target, std::string data) {
	if (routes.count(target) == 0)
		return false;
	routes[target]->transmit(data);
	return true;
}
