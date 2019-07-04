#include "RGateway.h"
#include "logging.h"
#include "query.h"
#include <map>

std::map<Address, RGateway*> remotes;

RGateway::RGateway(Address addr, Peer * from) : addr(addr), relay(from) {
	remotes[addr] = this;
	log("Received routing information for " + addr.stringify());
}

RGateway::~RGateway() {
	remotes.erase(addr);
	log("Lost routing information for " + addr.stringify());
}

RGateway * RGateway::lookup(Address req) {
	if (remotes.count(req) == 0) {
		query(req);

		if (remotes.count(req) == 0) {
			return nullptr;
		}
	}

	return remotes[req];
}

bool RGateway::sendTo(Address tgt, std::string data) {
	RGateway * gate = RGateway::lookup(tgt);

	if (gate == nullptr)
		return false;

	gate->relay->transmit(data);
	return true;
}

void RGateway::transmit(std::string data) {
	relay->transmit(data);
}

void RGateway::clean(Peer* target) {
	for (std::map<Address, RGateway*>::iterator iter = remotes.begin(); iter != remotes.end(); iter++) {
		if (iter->second->relay == target) {
			delete iter->second;
			remotes.erase(iter);
		}
	}
}
