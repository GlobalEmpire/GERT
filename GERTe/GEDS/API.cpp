/*
This is the code that provides protocol definition and formatting to the primary server.
This code should be compiled then placed in the "apis" subfolder of the main server.
This code is similarly cross-platform as the main server.
This code DEFINITELY supports Linux and MIGHT support MacOS (but the main server doesn't.)

This code is designed to function with other code from
https://github.com/GlobalEmpire/GERT/

This code falls under the license located at
https://github.com/GlobalEmpire/GERT/blob/master/License.md
*/

#include "API.h"
#include "Version.h"
#include "logging.h"
#include "NetString.h"
#include "Key.h"
#include "GERTc.h"
#include "gatewayManager.h"
#include "peerManager.h"
#include "routeManager.h"
#include "query.h"
using namespace std;
typedef unsigned char UCHAR;

extern void addResolution(Address, Key);
extern void removeResolution(Address);

enum gedsCommands {
	REGISTERED,
	UNREGISTERED,
	ROUTE,
	RESOLVE,
	UNRESOLVE,
	LINK,
	UNLINK,
	CLOSEPEER,
	QUERY
};

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

bool isLocal(Address addr) {
	try {
		Gateway* test = Gateway::lookup(addr);
		return true;
	}
	catch (int e) {
		return false;
	}
}

void changeState(Gateway * gate, const Gate::States newState, const char numextra = 0, const char * extra = nullptr) {
	gate->state = (char)newState;

	string update = string{ (char)newState };
	update += (char)newState;
	update += string{ extra, (size_t)numextra };

	gate->transmit(update);
}

void failed(Gateway * gate, const Gate::Errors error) {
	string response = string{ (char)Gate::Commands::STATE };
	response += (char)Gate::States::FAILURE;
	response += (char)error;

	gate->transmit(response);
}

void globalChange(const GEDS::Commands change, const char * parameter, const char len) {
	string data = string{ (char)change };
	data += string{ parameter, (size_t)len };

	broadcast(data);
}

constexpr UCHAR major = 1;
constexpr UCHAR minor = 0;
constexpr UCHAR patch = 0;
constexpr char vers[3] = { major, minor, patch };

Version ThisVers = {
	processGateway,
	processGEDS,
	nullptr,
	nullptr,
	{
		major,
		minor,
		patch
	},
	nullptr
};

void processGateway(Gateway* gate) {
	if (gate->state == (char)Gate::States::FAILURE) {
		changeState(gate, Gate::States::CONNECTED, 3, vers);
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
	Gate::Commands command = (Gate::Commands)(gate->read(1))[1];
	switch (command) {
	case Gate::Commands::REGISTER: {
		Address request = Address::extract(gate);
		Key requestkey = Key::extract(gate);
		if (gate->state == (char)Gate::States::REGISTERED) {
			failed(gate, Gate::Errors::REGISTERED);
			/*
			 * Response to registration attempt when gateway has address assigned.
			 * CMD STATE (0)
			 * STATE FAILURE (0)
			 * REASON ALREADY_REGISTERED (2)
			 */
			return;
		}
		if (isLocal(request) || isRemote(request)) {
			warn("Gateway attempted to claim " + request.stringify() + " but it was already taken");
			failed(gate, Gate::Errors::ADDRESS_TAKEN);
			/*
			 * Response to registration attempt when address is already taken.
			 * CMD STATE (0)
			 * STATE FAILURE (0)
			 * REASON ADDRESS_TAKEN (5)
			 */
			return;
		}
		if (gate->assign(request, requestkey)) {
			changeState(gate, Gate::States::REGISTERED);
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
			failed(gate, Gate::Errors::BAD_KEY);
		/*
		 * Response to failed registration attempt.
		 * CMD STATE (0)
		 * STATE FAILURE (0)
		 * REASON BAD_KEY (1)
		 */
		return;
	}
	case Gate::Commands::DATA: {
		GERTc target = GERTc::extract(gate);
		Address source = Address::extract(gate);
		NetString data = NetString::extract(gate);
		if (gate->state == (char)Gate::States::CONNECTED) {
			failed(gate, Gate::Errors::NOT_REGISTERED);
			/*
			 * Response to data before registration
			 * CMD STATE (0)
			 * STATE FAILURE (0)
			 * REASON NOT_REGISTERED (3)
			 */
			break;
		}
		string newCmd = string{ (char)Gate::Commands::DATA } +target.tostring() + gate->addr.tostring() +
			source.tostring() + data.string();
		if (isRemote(target.external) || isLocal(target.external)) { //Target is remote
			sendToGateway(target.external, newCmd); //Send to target
		}
		else {
			if (queryWeb(target.external)) {
				sendToGateway(target.external, newCmd);
			}
			else {
				failed(gate, Gate::Errors::NO_ROUTE);
				/*
				 * Response to failed data send request.
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON NO_ROUTE (4)
				 */
				return;
			}
		}
		gate->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::SENT }));
		/*
		 * Response to successful data send request.
		 * CMD STATE (0)
		 * STATE SENT (5)
		 */
		return;
	}
	case Gate::Commands::STATE: {
		gate->transmit(string({ (char)Gate::Commands::STATE, (char)gate->state }));
		/*
		 * Response to state request
		 * CMD STATE (0)
		 * STATE (1 byte, unsigned)
		 * No following data
		 */
		return;
	}
	case Gate::Commands::CLOSE: {
		gate->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::CLOSED }));
		/*
		 * Response to close request.
		 * CMD STATE (0)
		 * STATE CLOSED (3)
		 */
		if (gate->state == (char)Gate::States::REGISTERED) {
			string addr = gate->addr.tostring();
			globalChange(GEDS::Commands::UNREGISTERED, addr.data(), addr.length());
			/*
			 * Broadcast that registered gateway left.
			 * CMD UNREGISTERED (1)
			 * Address (4 bytes)
			 */
		}
		gate->close();
	}
	}
}

void processGEDS(Peer* geds) {
	if (geds->state == 0) {
		geds->state = 1;
		geds->transmit(string({ (char)major, (char)minor, (char)patch }));
		/*
		 * Initial packet
		 * MAJOR VERSION
		 * MINOR VERSION
		 * PATCH VERSION
		 */
		return;
	}
	UCHAR command = (geds->read(1))[1];
	switch (command) {
	case ROUTE: {
		GERTc target = GERTc::extract(geds);
		GERTc source = GERTc::extract(geds);
		NetString data = NetString::extract(geds);
		string cmd = { (char)GEDS::Commands::ROUTE };
		cmd += target.tostring() + source.tostring() + data.string();
		if (!sendToGateway(target.external, cmd)) {
			string errCmd = { UNREGISTERED };
			errCmd += target.tostring();
			geds->transmit(errCmd);
		}
		return;
	}
	case REGISTERED: {
		Address target = Address::extract(geds);
		setRoute(target, geds);
		return;
	}
	case UNREGISTERED: {
		Address target = Address::extract(geds);
		removeRoute(target);
		return;
	}
	case RESOLVE: {
		Address target = Address::extract(geds);
		Key key = Key::extract(geds);
		addResolution(target, key);
		return;
	}
	case UNRESOLVE: {
		Address target = Address::extract(geds);
		removeResolution(target);
		return;
	}
	case LINK: {
		IP target = IP::extract(geds);
		Ports ports = Ports::extract(geds);
		addPeer(target, ports);
		return;
	}
	case UNLINK: {
		IP target = IP::extract(geds);
		removePeer(target);
		return;
	}
	case CLOSEPEER: {
		geds->transmit(string({ CLOSEPEER }));
		geds->close();
		return;
	}
	case QUERY: {
		Address target = Address::extract(geds);
		if (isLocal(target)) {
			string cmd = { REGISTERED };
			cmd += target.tostring();
			sendToGateway(target, cmd);
		}
		else {
			string cmd = { UNREGISTERED };
			cmd += target.tostring();
			geds->transmit(cmd);
		}
		return;
	}
	}
}
