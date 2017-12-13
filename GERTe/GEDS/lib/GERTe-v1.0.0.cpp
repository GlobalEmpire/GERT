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
		QUERY,
		TUNNEL,
		TUNNEL_DATA,
		TUNNEL_ERROR,
		TUNNEL_OK
	};
}

namespace Gate {
	enum class Commands : char {
		STATE,
		REGISTER,
		DATA,
		CLOSE,
		TUNNEL,
		TUNNEL_DATA
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
		ADDRESS_TAKEN,
		NO_TUNNEL
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

void changeState(Gateway * gate, const Gate::States newState, const char numextra=0, const char * extra=nullptr) {
	gate->state = (char)newState;

	string update = string{(char)Gate::Commands::STATE};
	update += (char)newState;
	update += string{extra, numextra};

	gate->transmit(update);
}

void failed(Gateway * gate, const Gate::Errors error) {
	string response = string{(char)Gate::Commands::STATE};
	response += (char)Gate::States::FAILURE;
	response += (char)error;

	gate->transmit(response);
}

void globalChange(const GEDS::Commands change, const char * parameter, const char len) {
	string data = string{(char)change};
	data += string{parameter, len};

	broadcast(data);
}

STARTEXPORT

DLLExport constexpr UCHAR major = 1;
DLLExport constexpr UCHAR minor = 1;
DLLExport constexpr UCHAR patch = 0;
constexpr char vers[3] = { major, minor, patch };

DLLExport void processGateway(Gateway* gate) {
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
			} else
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
			string newCmd = string{(char)Gate::Commands::DATA} + target.tostring() + gate->addr.tostring() +
					source.tostring() + data.tostring();
			if (isRemote(target.external) || isLocal(target.external)) { //Target is remote
				sendToGateway(target.external, newCmd); //Send to target
			} else {
				if (queryWeb(target.external)) {
					sendToGateway(target.external, newCmd);
				} else {
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
		case Gate::Commands::TUNNEL: {
			Address target = Address::extract(gate);
			Address source = gate->addr;
			char * raw = gate->read(6);
			string strict = {raw + 1, 6};
			delete raw;

			char * id = createTunnel(source, target);

			if (isLocal(target)) {
				string result = string{(char)Gate::Commands::TUNNEL};
				result += source.tostring();
				result.append(id, ID_LENGTH);
				result += strict;
				sendToGateway(target, result);
				result = string{(char)Gate::Commands::TUNNEL};
				result.append(id, ID_LENGTH);
				gate->transmit(result);
			}
			else if (isRemote(target) || queryWeb(target)) {
				string result = string{(char)GEDS::Commands::TUNNEL};
				result += target.tostring() + source.tostring();
				result.append(id, ID_LENGTH);
				result += strict;
				sendToGateway(target, result);
			}
			else {
				gate->transmit(string{
					(char)Gate::Commands::STATE, (char)Gate::States::FAILURE, (char)Gate::Errors::NO_ROUTE
				}); //Transmit error to gateway
				return;
			}
			delete id;
		}
		case Gate::Commands::TUNNEL_DATA: {
			char * id = gate->read(ID_LENGTH);
			NetString data = NetString::extract(gate);

			if (id[0] < ID_LENGTH) {
				return;
			}

			Address target = *getTunnel(gate->addr, id+1);
			if (target == nullptr) {
				gate->transmit({(char)Gate::Commands::STATE, (char)Gate::States::FAILURE, (char)Gate::Errors::NO_TUNNEL});
				return;
			}

			if (isLocal(target)) {
				string result = string{(char)Gate::Commands::TUNNEL_DATA};
				result.append(id+1, ID_LENGTH);
				result += data.tostring();
				sendToGateway(target, result);
			} else if (isRemote(target) || queryWeb(target)) {
				string result = string{(char)GEDS::Commands::TUNNEL_DATA};
				result += target.tostring();
				result.append(id+1, ID_LENGTH);
				result += data.tostring();
				sendToGateway(target, result);
			} else {
				gate->transmit({
					(char)Gate::Commands::STATE, (char)Gate::States::FAILURE, (char)Gate::Errors::NO_ROUTE
				});
				destroyTunnel(gate->addr, id+1);
			}
			delete id;
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
	GEDS::Commands command = (GEDS::Commands)(geds->read(1))[1];
	switch (command) {
		case GEDS::Commands::ROUTE: {
			GERTc target = GERTc::extract(geds);
			GERTc source = GERTc::extract(geds);
			NetString data = NetString::extract(geds);
			string cmd = { (char)GEDS::Commands::ROUTE };
			cmd += target.tostring() + source.tostring() + data.tostring();
			if (!sendToGateway(target.external, cmd)) {
				string errCmd = { (char)GEDS::Commands::UNREGISTERED };
				errCmd += target.tostring();
				geds->transmit(errCmd);
			}
			return;
		}
		case GEDS::Commands::REGISTERED: {
			Address target = Address::extract(geds);
			setRoute(target, geds);
			return;
		}
		case GEDS::Commands::UNREGISTERED: {
			Address target = Address::extract(geds);
			removeRoute(target);
			return;
		}
		case GEDS::Commands::RESOLVE: {
			Address target = Address::extract(geds);
			Key key = Key::extract(geds);
			addResolution(target, key);
			return;
		}
		case GEDS::Commands::UNRESOLVE: {
			Address target = Address::extract(geds);
			removeResolution(target);
			return;
		}
		case GEDS::Commands::LINK: {
			IP target = IP::extract(geds);
			Ports ports = Ports::extract(geds);
			addPeer(target, ports);
			return;
		}
		case GEDS::Commands::UNLINK: {
			IP target = IP::extract(geds);
			removePeer(target);
			return;
		}
		case GEDS::Commands::CLOSE: {
			geds->transmit(string({ (char)GEDS::Commands::CLOSE }));
			geds->close();
			return;
		}
		case GEDS::Commands::QUERY: {
			Address target = Address::extract(geds);
			if (isLocal(target)) {
				string cmd = { (char)GEDS::Commands::REGISTERED };
				cmd += target.tostring();
				sendToGateway(target, cmd);
			} else {
				string cmd = { (char)GEDS::Commands::UNREGISTERED };
				cmd += target.tostring();
				geds->transmit(cmd);
			}
			return;
		}
		case GEDS::Commands::TUNNEL: {
			Address target = Address::extract(geds);
			Address source = Address::extract(geds);
			char * id = geds->read(ID_LENGTH);
			char * raw = geds->read(6);
			string strict = string{raw + 1, 6};
			delete raw;

			if (isLocal(target) && getTunnel(target, id+1) == nullptr) {
				string result = string{(char)Gate::Commands::TUNNEL};
				result += source.tostring();
				result.append(id, ID_LENGTH);
				result += strict;
				sendToGateway(target, result);

				result = string{(char)GEDS::Commands::TUNNEL_OK};
				result.append(id+1, ID_LENGTH);
				geds->transmit(result);

				addTunnel(target, source, id+1);
			} else {
				string result = string{(char)GEDS::Commands::TUNNEL_ERROR};
				result.append(id+1, ID_LENGTH);
				geds->transmit(result);
			}
			delete id;
		}
		case GEDS::Commands::TUNNEL_DATA: {
			Address target = Address::extract(geds);
			char * id = geds->read(ID_LENGTH);
			NetString data = NetString::extract(geds);

			if (id[0] < ID_LENGTH) {
				return;
			}

			if (getTunnel(target, id+1) == nullptr) {
				string result = string{(char)GEDS::Commands::TUNNEL_ERROR};
				result.append(id+1, ID_LENGTH);
				geds->transmit(result);
				return;
			}

			if (isLocal(target)) {
				string result = string{(char)Gate::Commands::TUNNEL_DATA};
				result.append(id+1, ID_LENGTH);
				result += data.tostring();
				sendToGateway(target, result);
			} else {
				string result = string{(char)GEDS::Commands::TUNNEL_ERROR};
				result.append(id+1, ID_LENGTH);
				geds->transmit(result);
				destroyTunnel(target, id+1);
			}
			delete id;
		}
	}
}

DLLExport void killGateway(Gateway* gate) {
	gate->transmit(string({ (char)Gate::Commands::CLOSE })); //SEND CLOSE REQUEST
	gate->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::CLOSED })); //SEND STATE UPDATE TO CLOSED (0, 3)
	gate->close();
}

DLLExport void killGEDS(Peer* geds) {
	geds->transmit(string({ (char)GEDS::Commands::CLOSE })); //SEND CLOSE REQUEST
	geds->close();
}

ENDEXPORT

#ifdef _WIN32
bool WINAPI DllMain(HINSTANCE w, DWORD a, LPVOID s) {
	return true;
}
#endif
