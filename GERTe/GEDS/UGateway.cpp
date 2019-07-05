#include "logging.h"
#include "Poll.h"
#include "GERTc.h"
#include "NetString.h"
#include "query.h"
#include "Versioning.h"
#include "Gateway.h"
#include "Peer.h"
#include "RGateway.h"
#include <map>
#include <fcntl.h>
using namespace std;

map<Address, Gateway*> gateways;
vector<UGateway*> noAddrList;

extern Poll netPoll;

void changeState(UGateway * gate, const Gate::States newState, const char numextra = 0, const char * extra = nullptr) {
	gate->state = (char)newState;

	string update = string{ (char)Gate::Commands::STATE };
	update += (char)newState;
	update += string{ extra, (size_t)numextra };

	gate->transmit(update);
}

void globalChange(const GEDS::Commands change, const char * parameter, const char len) {
	string data = string{ (char)change };
	data += string{ parameter, (size_t)len };

	Peer::broadcast(data);
}

void failed(UGateway * gate, const Gate::Errors error) {
	string response = string{ (char)Gate::Commands::STATE };
	response += (char)Gate::States::FAILURE;
	response += (char)error;

	gate->transmit(response);
}

bool removeNoAddr(UGateway* gate) {
	for (vector<UGateway*>::iterator iter = noAddrList.begin(); iter != noAddrList.end(); iter++) { //Loop through list of non-registered gateways
		if (*iter == gate) { //If we found the Gateway
			noAddrList.erase(iter);
			return true;
		}
	}

	return false;
}

UGateway::UGateway(SOCKET newSock) : Connection(newSock, "Gateway") {
	noAddrList.push_back(this);
	changeState(this, Gate::States::CONNECTED, 2, (char*)&ThisVersion);

	if (vers[1] == 0)
		transmit("\0");

	last = (char)Gate::Commands::STATE;										// Needs something that never calls consume
}

UGateway::~UGateway() {
	if (removeNoAddr(this))
		log("Unregistered gateway has closed the connection");

	netPoll.remove(this);
}

UGateway::UGateway(UGateway&& orig) noexcept : Connection(orig) {
	removeNoAddr(&orig);
	std::free(&orig);
}

void UGateway::transmit(string data) {
	send(sock, data.c_str(), (ULONG)data.length(), 0);
}

bool UGateway::assign(Address requested, Key key) {
	if (key.check(requested)) { //Determine if the key is for the address
		log("Association from " + requested.stringify()); //Notify user that the address has registered
		return true; //Notify the protocol library assignment was successful
	}
	return false; //Notify the protocol library assignment has failed
}

void UGateway::close() {
	transmit(string({ (char)Gate::Commands::CLOSE })); //SEND CLOSE REQUEST
	transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::CLOSED })); //SEND STATE UPDATE TO CLOSED (0, 3)
	//Used to free memory, ENSURE REFERENCES ARE PATCHED
}

void UGateway::process(Gateway * derived) {
	if (last == (char)Gate::Commands::STATE) {
		consume(1);
		last = buf[0];
		clean();
	}

	Gate::Commands command = (Gate::Commands)last;

	switch (command) {
	case Gate::Commands::REGISTER:
		if (consume(23)) {
			Address request{ this, 0 };
			Key requestkey{ this, 3 };
			clean();

			last = (char)Gate::Commands::STATE;

			if (state == (char)Gate::States::REGISTERED) {
				failed(this, Gate::Errors::REGISTERED);
				/*
				 * Response to registration attempt when Gateway has address assigned.
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON ALREADY_REGISTERED (2)
				 */
				return;
			}
			if (Gateway::lookup(request) || RGateway::lookup(request)) {
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
			if (assign(request, requestkey)) {
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

				Gateway* assigned = new Gateway{ request, this };
			}
			else
				failed(this, Gate::Errors::BAD_KEY);
			/*
			 * Response to failed registration attempt.
			 * CMD STATE (0)
			 * STATE FAILURE (0)
			 * REASON BAD_KEY (1)
			 */
		}
		return;
	case Gate::Commands::DATA:
		if (consume(9, true)) {
			GERTc target{ this, 0 };
			Address source{ this, 6 };
			NetString data{ this, 9 };
			clean();

			last = (char)Gate::Commands::STATE;

			if (state == (char)Gate::States::CONNECTED) {
				failed(this, Gate::Errors::NOT_REGISTERED);
				/*
				 * Response to data before registration
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON NOT_REGISTERED (3)
				 */
				return;;
			}
			string newCmd = string{ (char)Gate::Commands::DATA } +target.tostring() + derived->addr.tostring() + source.tostring() + data.string();
			if (RGateway::lookup(target.external) || Gateway::lookup(target.external)) { //Target is remote
				Gateway::sendTo(target.external, newCmd); //Send to target
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
			transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::SENT }));
			/*
			 * Response to successful data send request.
			 * CMD STATE (0)
			 * STATE SENT (5)
			 */
		}
		return;
	case Gate::Commands::STATE: {
		transmit(string({ (char)Gate::Commands::STATE, (char)this->state }));
		/*
		 * Response to state request
		 * CMD STATE (0)
		 * STATE (1 byte, unsigned)
		 * No following data
		 */
		return;
	}
	case Gate::Commands::CLOSE:
		transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::CLOSED }));
		/*
		 * Response to close request.
		 * CMD STATE (0)
		 * STATE CLOSED (3)
		 */
		delete this;
	}
}
