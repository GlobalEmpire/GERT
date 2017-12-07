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

#include "libHelper.h"
using namespace std;

typedef unsigned char UCHAR;

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
	} catch (int e) {
		return false;
	}
}

STARTEXPORT

DLLExport UCHAR major = 1;
DLLExport UCHAR minor = 0;
DLLExport UCHAR patch = 0;

DLLExport void processGateway(Gateway* gate) {
	if (gate->state == (char)Gate::States::FAILURE) {
		gate->state = (char)Gate::States::CONNECTED;
		gate->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::CONNECTED, (char)major, (char)minor, (char)patch }));
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
				string cmd{(char)Gate::Commands::STATE, (char)Gate::States::FAILURE, (char)Gate::Errors::REGISTERED};
				gate->transmit(cmd);
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
				string cmd{(char)Gate::Commands::STATE, (char)Gate::States::FAILURE, (char)Gate::Errors::ADDRESS_TAKEN};
				gate->transmit(cmd);
				/*
				 * Response to registration attempt when address is already taken.
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON ADDRESS_TAKEN (5)
				 */
				return;
			}
			if (gate->assign(request, requestkey)) {
				gate->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::REGISTERED }));
				gate->state = (char)Gate::States::REGISTERED;
				/*
				 * Response to successful registration attempt
				 * CMD STATE (0)
				 * STATE REGISTERED (2)
				 */
				string cmd = {(char)GEDS::Commands::REGISTERED};
				cmd += request.tostring();
				broadcast(cmd);
				/*
				 * Broadcast to all peers registration
				 * CMD REGISTERED (0)
				 * Address (4 bytes)
				 */
			} else
				gate->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::FAILURE, (char)Gate::Errors::BAD_KEY }));
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

				gate->transmit(string({(char)Gate::Commands::STATE, (char)Gate::States::FAILURE, (char)Gate::Errors::NOT_REGISTERED}));
				/*
				 * Response to data before registration
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON NOT_REGISTERED (3)
				 */
				break;
			}
			string newCmd = string{(char)Gate::Commands::DATA} + target.tostring() + gate->addr.tostring() +
					source.tostring() + data.tostring();
			if (isRemote(target.external) || isLocal(target.external)) { //Target is remote
				sendToGateway(target.external, newCmd); //Send to target
			} else {
				if (queryWeb(target.external)) {
					sendToGateway(target.external, newCmd);
				} else {
					gate->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::FAILURE, (char)Gate::Errors::NO_ROUTE }));
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
				string cmd = {(char)GEDS::Commands::UNREGISTERED};
				cmd += gate->addr.tostring();
				broadcast(cmd);
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

DLLExport void processGEDS(Peer* geds) {
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
			cmd += target.tostring() + source.tostring() + data.tostring();
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
			} else {
				string cmd = { UNREGISTERED };
				cmd += target.tostring();
				geds->transmit(cmd);
			}
			return;
		}
	}
}

DLLExport void killGateway(Gateway* gate) {
	gate->transmit(string({ (char)Gate::Commands::CLOSE })); //SEND CLOSE REQUEST
	gate->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::CLOSED })); //SEND STATE UPDATE TO CLOSED (0, 3)
	gate->close();
}

DLLExport void killGEDS(Peer* geds) {
	geds->transmit(string({ CLOSEPEER })); //SEND CLOSE REQUEST
	geds->close();
}

ENDEXPORT

#ifdef _WIN32
bool WINAPI DllMain(HINSTANCE w, DWORD a, LPVOID s) {
	return true;
}
#endif
