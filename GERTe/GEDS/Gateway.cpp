#include "Gateway.h"
#include "gatewayManager.h"
#include "routeManager.h"
#include "logging.h"
#include <utility>
#include "Poll.h"

extern std::map<Address, Gateway*> gateways;
extern Poll gatePoll;

Gateway::Gateway(Address addr, UGateway * orig) : UGateway(std::move(*orig)), addr(addr) {
	gateways[addr] = this;

	gatePoll.remove(sock);
	gatePoll.add(sock);
}

Gateway::~Gateway() {
	gateways.erase(this->addr);
	log("Disassociation from " + this->addr.stringify());
}

Gateway* Gateway::lookup(Address req) {
	if (gateways.count(req) == 0) {
		return nullptr;
	}

	return gateways[req];
}

bool Gateway::sendTo(Address addr, std::string data) {
	if (gateways.count(addr) != 0) { //If that Gateway is in the database
		gateways[addr]->transmit(data);
		return true; //Notify the protocol library that we succeeded in sending
	}
	return RGateway::sendTo(addr, data); //Attempt to send to Gateway with address via routing. Notify protocol library of result.
}

void Gateway::process() {
	UGateway::process(this);
}
