/*
This is the code that provides protocol definition and formatting to the primary server.
This code should be compiled then placed in the "apis" subfolder of the main server.
This code is similarly cross-platform as the main server.
This code DEFINITELY supports Windows and Linux and MIGHT support MacOS (but the main server doesn't.)

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

enum gatewayCommands {
	STATE,
	REGISTER,
	DATA,
	STATUS,
	CLOSE
};

enum gatewayStates {
	FAILURE,
	CONNECTED,
	ASSIGNED,
	CLOSED,
	SENT
};

enum gatewayErrors {
	VERSION,
	BAD_KEY,
	ALREADY_REGISTERED,
	NOT_REGISTERED,
	NO_ROUTE
};

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

STARTEXPORT

DLLExport UCHAR major = 1;
DLLExport UCHAR minor = 0;
DLLExport UCHAR patch = 0;

DLLExport void processGateway(gateway* gate, string packet) {
	if (gate->state == FAILURE) {
		gate->state = CONNECTED;
		sendTo(gate, string({ STATE, CONNECTED, (char)major, (char)minor, (char)patch }));
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
	UCHAR command = packet.data()[0];
	string rest = packet.erase(0, 1);
	switch (command) {
		case REGISTER: {
			if (gate->state == ASSIGNED) {
				sendTo(gate, string({ STATE, FAILURE, ALREADY_REGISTERED }));
				/*
				 * Response to registration attempt when gateway has address assigned.
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON ALREADY_REGISTERED (2)
				 */
				return;
			}
			GERTaddr request = getAddr(rest);
			rest.erase(0, 4);
			GERTkey requestkey(rest);
			if (assign(gate, request, requestkey)) {
				sendTo(gate, string({ STATE, ASSIGNED }));
				gate->state = REGISTERED;
				/*
				 * Response to successful registration attempt
				 * CMD STATE (0)
				 * STATE REGISTERED (2)
				 */
				string cmd = {REGISTERED};
				cmd += rest.substr(0, 4);
				broadcast(cmd);
				/*
				 * Broadcast to all peers registration
				 * CMD REGISTERED (0)
				 * GERTaddr (4 bytes)
				 */
			} else
				sendTo(gate, string({ STATE, FAILURE, BAD_KEY }));
				/*
				 * Response to failed registration attempt.
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON BAD_KEY (1)
				 */
			return;
		}
		case DATA: {
			if (gate->state == CONNECTED) {
				sendTo(gate, string({STATE, FAILURE, NOT_REGISTERED}));
				/*
				 * Response to data before registration
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON NOT_REGISTERED (3)
				 */
				break;
			}
			GERTaddr target = getAddr(rest); //Assign target address as first 4 bytes
			string newCmd = string{DATA} + rest;
			if (isRemote(target) || isLocal(target)) { //Target is remote
				sendTo(target, newCmd); //Send to target
			} else {
				if (queryWeb(target)) {
					sendTo(target, newCmd);
				} else {
					sendTo(gate, string({ STATE, FAILURE, NO_ROUTE }));
					/*
					 * Response to failed data send request.
					 * CMD STATE (0)
					 * STATE FAILURE (0)
					 * REASON NO_ROUTE (4)
					 */
					return;
				}
			}
			sendTo(gate, string({ STATE, SENT }));
			/*
			 * Response to successful data send request.
			 * CMD STATE (0)
			 * STATE SENT (5)
			 */
			return;
		}
		case STATUS: {
			sendTo(gate, string({ STATE, (char)gate->state }));
			/*
			 * Response to state request
			 * CMD STATE (0)
			 * STATE (1 byte, unsigned)
			 * No following data
			 */
			return;
		}
		case CLOSE: {
			sendTo(gate, string({ STATE, CLOSED }));
			/*
			 * Response to close request.
			 * CMD STATE (0)
			 * STATE CLOSED (3)
			 */
			if (gate->state == REGISTERED) {
				string cmd = {UNREGISTERED};
				cmd += putAddr(gate->addr);
				broadcast(cmd);
				removeResolution(gate->addr);
				/*
				 * Broadcast that registered gateway left.
				 * CMD UNREGISTERED (1)
				 * GERTaddr (4 bytes)
				 */
			}
			closeTarget(gate);
		}
	}
}

DLLExport void processGEDS(peer* geds, string packet) {
	if (geds->state == 0) {
		geds->state = 1;
		sendTo(geds, string({ (char)major, (char)minor, (char)patch }));
		/*
		 * Initial packet
		 * MAJOR VERSION
		 * MINOR VERSION
		 * PATCH VERSION
		 */
		return;
	}
	UCHAR command = packet.data()[0];
	string rest = packet.erase(0, 1);
	switch (command) {
		case ROUTE: {
			GERTaddr target = getAddr(rest);
			string cmd = { DATA };
			cmd += rest;
			if (!sendTo(target, cmd)) {
				string errCmd = { UNREGISTERED };
				errCmd += putAddr(target);
				sendTo(geds, errCmd);
			}
			return;
		}
		case REGISTERED: {
			GERTaddr target = getAddr(rest);
			setRoute(target, geds);
			return;
		}
		case UNREGISTERED: {
			GERTaddr target = getAddr(rest);
			removeRoute(target);
			return;
		}
		case RESOLVE: {
			GERTaddr target = getAddr(rest);
			rest.erase(0, 4);
			GERTkey key = rest;
			addResolution(target, key);
			return;
		}
		case UNRESOLVE: {
			GERTaddr target = getAddr(rest);
			removeResolution(target);
			return;
		}
		case LINK: {
			ipAddr target(rest);
			rest.erase(0, 4);
			portComplex ports = makePorts(rest);
			addPeer(target, ports);
			return;
		}
		case UNLINK: {
			ipAddr target(rest);
			removePeer(target);
			return;
		}
		case CLOSEPEER: {
			sendTo(geds, string({ CLOSEPEER }));
			closeTarget(geds);
			return;
		}
		case QUERY: {
			GERTaddr target = getAddr(rest);
			if (isLocal(target)) {
				string cmd = { REGISTERED };
				cmd += putAddr(target);
				sendTo(target, cmd);
			} else {
				string cmd = { UNREGISTERED };
				cmd += putAddr(target);
				sendTo(geds, cmd);
			}
			return;
		}
	}
}

DLLExport void killGateway(gateway* gate) {
	sendTo(gate, string({ CLOSE })); //SEND CLOSE REQUEST
	sendTo(gate, string({ STATE, CLOSED })); //SEND STATE UPDATE TO CLOSED (0, 3)
	closeTarget(gate);
}

DLLExport void killGEDS(peer* geds) {
	sendTo(geds, string({ CLOSEPEER })); //SEND CLOSE REQUEST
	closeTarget(geds);
}

ENDEXPORT

#ifdef _WIN32
bool WINAPI DllMain(HINSTANCE w, DWORD a, LPVOID s) {
	return true;
}
#endif
