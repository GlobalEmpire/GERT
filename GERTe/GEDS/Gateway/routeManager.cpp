#include "routeManager.h"
#include "../Util/logging.h"

extern std::map<Address, RGateway*> remotes;

bool routeIter::isEnd() { return (ptr == remotes.end()) || (ptr == remotes.end()); }
routeIter routeIter::operator++ (int a) { return (ptr++, *this); }
routeIter::routeIter() : ptr(remotes.begin()) {};
routePtr routeIter::operator-> () { return ptr; }

void killAssociated(Peer* target) {
	for (routeIter iter; !iter.isEnd(); iter++) {
		if (iter->second->relay == target) {
			RGateway* toDie = RGateway::lookup(iter->first);
			delete toDie;
			remotes.erase(iter->first);
		}
	}
}
