#include "Gateway.h"
#include "gatewayManager.h"
#include "routeManager.h"
#include "logging.h"
#include <utility>
#include "Poll.h"

extern std::map<Address, Gateway*> gateways;
extern Poll gatePoll;

enum class GateCommands : char {
	STATE,
	REGISTER,
	DATA,
	CLOSE,
	TUNNEL_START,
	TUNNEL_DATA,
	TUNNEL_END
};

enum class GEDSCommands : char {
	REGISTERED,
	UNREGISTERED,
	ROUTE,
	RESOLVE,
	UNRESOLVE,
	LINK,
	UNLINK,
	CLOSE,
	QUERY,
	TUNNEL_START,
	TUNNEL_OPEN,
	TUNNEL_DATA,
	TUNNEL_END
};

Gateway::Gateway(Address addr, UGateway * orig) : UGateway(std::move(*orig)), addr(addr) {
	gateways[addr] = this;

	gatePoll.remove(sock);
	gatePoll.add(sock, this);
}

Gateway::~Gateway() {
	gateways.erase(this->addr);
	log("Disassociation from " + this->addr.stringify());

	for (std::pair<uint16_t, Tunnel> pair : tunnels) {
		if (pair.second.local.external == addr || pair.second.remote.external == addr)
			closeTunnel(pair.first);
	}
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

void Gateway::closeTunnel(uint16_t tunId) {
	union {
		uint16_t num;
		char bytes[2];
	} netTunL;

	netTunL.num = ntohs(tunId);

	std::string newCmd({ (char)GateCommands::TUNNEL_END, netTunL.bytes[0], netTunL.bytes[1] });

	if (tunnels.count(tunId) != 0) {
		Tunnel tun = tunnels[tunId];
		Address target = tun.remote.external;
		if (target == addr)
			target = tun.local.external;

		union {
			uint16_t num;
			char bytes[2];
		} netTunR;

		netTunR.num = tun.remoteId;

		if (RGateway* remote = RGateway::lookup(target)) { //Target is remote
			Peer* relay = remote->relay;
			std::string remoteCmd({ (char)GEDSCommands::TUNNEL_END, netTunR.bytes[0], netTunR.bytes[1] });
			relay->transmit(remoteCmd);
		}
		else if (Gateway* remote = Gateway::lookup(target)) {
			remote->transmit(newCmd);
		}

		tunnels.erase(tunId);
	}

	transmit(newCmd);
}
