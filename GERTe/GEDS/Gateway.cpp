#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#endif

#include "netty.h"
#include "gatewayManager.h"
#include "logging.h"
#include <fcntl.h>
#include <map>
#include "Poll.h"
#include "routeManager.h"
#include "peerManager.h"
#include "GERTc.h"
#include "NetString.h"
#include "query.h"

using namespace std;

extern map<Address, Key> resolutions;

extern Poll gatePoll;

namespace Gate {
	enum class Commands : char {
		STATE,
		REGISTER,
		DATA,
		CLOSE
	};

	enum class States : char {
		FAILURE,
		CONNECTED,
		REGISTERED,
		CLOSED,
		SENT
	};

	enum class Errors : char {
		VERSION,
		BAD_KEY,
		REGISTERED,
		NOT_REGISTERED,
		NO_ROUTE,
		ADDRESS_TAKEN
	};
}

namespace GEDS {
	enum class Commands : char {
		REGISTERED,
		UNREGISTERED,
		ROUTE,
		RESOLVE,
		UNRESOLVE,
		LINK,
		UNLINK,
		CLOSE,
		QUERY
	};
}

extern Versioning ThisVersion;

map<Address, Gateway*> gateways;
vector<Gateway*> noAddrList;

void changeState(Gateway * gate, const Gate::States newState, const char numextra = 0, const char * extra = nullptr) {
	gate->state = (char)newState;

	string update = string{ (char)newState };
	update += (char)newState;
	update += string{ extra, (size_t)numextra };

	gate->transmit(update);
}

void globalChange(const GEDS::Commands change, const char * parameter, const char len) {
	string data = string{ (char)change };
	data += string{ parameter, (size_t)len };

	broadcast(data);
}

void failed(Gateway * gate, const Gate::Errors error) {
	string response = string{ (char)Gate::Commands::STATE };
	response += (char)Gate::States::FAILURE;
	response += (char)error;

	gate->transmit(response);
}

noAddrIter find(Gateway * gate) {
	noAddrIter iter;
	for (iter; !iter.isEnd(); iter++) { //Loop through list of non-registered gateways
		if (*iter == gate) { //If we found the Gateway
			return iter;
		}
	}
	return iter;
}

Gateway::Gateway(void* sock) : Connection(sock, "Gateway") {
	local = true;
	noAddrList.push_back(this);
	process(); //Process empty data (Protocol Library Gateway Initialization)
}

Gateway::~Gateway() {
	if (local) {
		noAddrIter pos = find(this);
		pos.erase();
	}
	else {
		gateways.erase(this->addr); //Remove connection from universal map
		log("Disassociation from " + this->addr.stringify()); //Notify the user of the closure
	}

	gatePoll.remove(*(SOCKET*)sock);

	if (this->sock != nullptr)
		destroy((SOCKET*)this->sock); //Close the socket
}

Gateway* Gateway::lookup(Address req) {
	if (gateways.count(req) == 0) {
		return nullptr;
	}

	return gateways[req];
}

void Gateway::transmit(string data) {
	send(*(SOCKET*)this->sock, data.c_str(), (ULONG)data.length(), 0);
}

bool Gateway::assign(Address requested, Key key) {
	if (resolutions[requested] == key) { //Determine if the key is for the address
		this->addr = requested; //Set the address
		gateways[requested] = this; //Add Gateway to the database
		noAddrIter pos = find(this);
		pos.erase();
		log("Association from " + requested.stringify()); //Notify user that the address has registered
		return true; //Notify the protocol library assignment was successful
	}
	return false; //Notify the protocol library assignment has failed
}

void Gateway::close() {
	this->transmit(string({ (char)Gate::Commands::CLOSE })); //SEND CLOSE REQUEST
	this->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::CLOSED })); //SEND STATE UPDATE TO CLOSED (0, 3)
	delete this; //Release the memory used to store the Gateway
}

void Gateway::process() {
	if (this->state == (char)Gate::States::FAILURE) {
		changeState(this, Gate::States::CONNECTED, 3, &ThisVersion);
		/*
		 * Response to connection attempt.
		 * CMD STATE (0)
		 * STATE CONNECTED (1)
		 * MAJOR VERSION
		 * MINOR VERSION
		 * PATCH VERSION
		 */
		return;
	}
	Gate::Commands command = (Gate::Commands)(this->read(1))[1];
	switch (command) {
	case Gate::Commands::REGISTER: {
		Address request = Address::extract(this);
		Key requestkey = Key::extract(this);
		if (this->state == (char)Gate::States::REGISTERED) {
			failed(this, Gate::Errors::REGISTERED);
			/*
			 * Response to registration attempt when Gateway has address assigned.
			 * CMD STATE (0)
			 * STATE FAILURE (0)
			 * REASON ALREADY_REGISTERED (2)
			 */
			return;
		}
		if (Gateway::lookup(request) || isRemote(request)) {
			warn("Gateway attempted to claim " + request.stringify() + " but it was already taken");
			failed(this, Gate::Errors::ADDRESS_TAKEN);
			/*
			 * Response to registration attempt when address is already taken.
			 * CMD STATE (0)
			 * STATE FAILURE (0)
			 * REASON ADDRESS_TAKEN (5)
			 */
			return;
		}
		if (this->assign(request, requestkey)) {
			changeState(this, Gate::States::REGISTERED);
			/*
			 * Response to successful registration attempt
			 * CMD STATE (0)
			 * STATE REGISTERED (2)
			 */
			string addr = request.tostring();
			globalChange(GEDS::Commands::REGISTERED, addr.data(), addr.length());
			/*
			 * Broadcast to all peers registration
			 * CMD REGISTERED (0)
			 * Address (4 bytes)
			 */
		}
		else
			failed(this, Gate::Errors::BAD_KEY);
		/*
		 * Response to failed registration attempt.
		 * CMD STATE (0)
		 * STATE FAILURE (0)
		 * REASON BAD_KEY (1)
		 */
		return;
	}
	case Gate::Commands::DATA: {
		GERTc target = GERTc::extract(this);
		Address source = Address::extract(this);
		NetString data = NetString::extract(this);
		if (this->state == (char)Gate::States::CONNECTED) {
			failed(this, Gate::Errors::NOT_REGISTERED);
			/*
			 * Response to data before registration
			 * CMD STATE (0)
			 * STATE FAILURE (0)
			 * REASON NOT_REGISTERED (3)
			 */
			break;
		}
		string newCmd = string{ (char)Gate::Commands::DATA } +target.tostring() + this->addr.tostring() + source.tostring() + data.string();
		if (isRemote(target.external) || Gateway::lookup(target.external)) { //Target is remote
			sendToGateway(target.external, newCmd); //Send to target
		}
		else {
			if (queryWeb(target.external)) {
				sendToGateway(target.external, newCmd);
			}
			else {
				failed(this, Gate::Errors::NO_ROUTE);
				/*
				 * Response to failed data send request.
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON NO_ROUTE (4)
				 */
				return;
			}
		}
		this->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::SENT }));
		/*
		 * Response to successful data send request.
		 * CMD STATE (0)
		 * STATE SENT (5)
		 */
		return;
	}
	case Gate::Commands::STATE: {
		this->transmit(string({ (char)Gate::Commands::STATE, (char)this->state }));
		/*
		 * Response to state request
		 * CMD STATE (0)
		 * STATE (1 byte, unsigned)
		 * No following data
		 */
		return;
	}
	case Gate::Commands::CLOSE: {
		this->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::CLOSED }));
		/*
		 * Response to close request.
		 * CMD STATE (0)
		 * STATE CLOSED (3)
		 */
		if (this->state == (char)Gate::States::REGISTERED) {
			string addr = this->addr.tostring();
			globalChange(GEDS::Commands::UNREGISTERED, addr.data(), addr.length());
			/*
			 * Broadcast that registered Gateway left.
			 * CMD UNREGISTERED (1)
			 * Address (4 bytes)
			 */
		}
		delete this;
	}
	}
}
