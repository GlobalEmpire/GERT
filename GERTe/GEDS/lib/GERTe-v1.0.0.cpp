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

#ifdef _WIN32
#define DLLExport __declspec(dllexport)
#else
#define DLLExport
#endif

#define STARTEXPORT extern "C" {
#define ENDEXPORT }

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
		STATUS,
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

string extract(Connection * conn, unsigned char len) {
	char * buf = conn->read(len);
	string data{buf+1, buf[0]};
	delete[] buf;
	return data;
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
			string rest = extract(gate, 23);
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
			Address request{rest};
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
			rest.erase(0, 3);
			Key requestkey{rest};
			if (gate->assign(request, requestkey)) {
				gate->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::REGISTERED }));
				gate->state = (char)Gate::States::REGISTERED;
				/*
				 * Response to successful registration attempt
				 * CMD STATE (0)
				 * STATE REGISTERED (2)
				 */
				string cmd = {(char)GEDS::Commands::REGISTERED};
				cmd += putAddr(request);
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
			string rest = extract(gate, 9);
			char * len = gate->read(1);
			string data = extract(gate, len[1]);
			rest += (char)data.size() + data;
			delete len;
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
			GERTc target{rest}; //Assign target address as first 4 bytes
			rest.erase(0, 6);
			Address source{rest};
			rest.erase(0, 3);
			string newCmd = string{(char)Gate::Commands::DATA} + putAddr(target.external) + putAddr(target.internal) +
					putAddr(gate->addr) + putAddr(source) + rest;
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
		case Gate::Commands::STATUS: {
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
				cmd += putAddr(gate->addr);
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
			string rest = extract(geds, 12);
			char * len = geds->read();
			string data = extract(geds, *len);
			rest += (char)data.size() + data;
			delete len;
			Address target{rest};
			string cmd = { (char)GEDS::Commands::ROUTE };
			cmd += rest;
			if (!sendToGateway(target, cmd)) {
				string errCmd = { UNREGISTERED };
				errCmd += putAddr(target);
				geds->transmit(errCmd);
			}
			return;
		}
		case REGISTERED: {
			string rest = extract(geds, 3);
			Address target{rest};
			setRoute(target, geds);
			return;
		}
		case UNREGISTERED: {
			string rest = extract(geds, 3);
			Address target{rest};
			removeRoute(target);
			return;
		}
		case RESOLVE: {
			string rest = extract(geds, 3);
			Address target{rest};
			rest.erase(0, 6);
			Key key = rest;
			addResolution(target, key);
			return;
		}
		case UNRESOLVE: {
			string rest = extract(geds, 3);
			Address target{rest};
			removeResolution(target);
			return;
		}
		case LINK: {
			string rest = extract(geds, 8);
			IP target(rest);
			rest.erase(0, 4);
			unsigned short * ptr = (unsigned short*)rest.data();
			Ports ports{ptr[0], ptr[1]};
			addPeer(target, ports);
			return;
		}
		case UNLINK: {
			string rest = extract(geds, 4);
			IP target(rest);
			removePeer(target);
			return;
		}
		case CLOSEPEER: {
			geds->transmit(string({ CLOSEPEER }));
			geds->close();
			return;
		}
		case QUERY: {
			string rest = extract(geds, 3);
			Address target{rest};
			if (isLocal(target)) {
				string cmd = { REGISTERED };
				cmd += putAddr(target);
				sendToGateway(target, cmd);
			} else {
				string cmd = { UNREGISTERED };
				cmd += putAddr(target);
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
