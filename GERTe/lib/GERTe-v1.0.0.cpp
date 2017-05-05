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
	QUERY,
	CLOSE
};

enum gatewayStates {
	FAILURE,
	CONNECTED,
	ASSIGNED,
	DENIED,
	CLOSED,
	SENT
};

enum gatewayErrors {
	VERSION,
	BAD_KEY,
	ALREADY_REGISTERED,
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
	CLOSEPEER
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
	USHORT* restShort = (USHORT*)rest.c_str();
	switch (command) {
		case REGISTER: {
			if (gate->state == ASSIGNED) {
				sendTo(gate, string({ STATE, FAILURE, ALREADY_REGISTERED }));
				/*
				 * Response to registration attempt when gateway has address assigned.
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON ALREADY_REGISTERED (3)
				 */
				return;
			}
			GERTaddr request = {restShort[0], restShort[1]};
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
			GERTaddr target = {restShort[0], restShort[1]}; //Assign target address as first 4 bytes
			restShort[0] = gate->addr.high; //Set first 4 bytes to source address
			restShort[1] = gate->addr.low;
			rest.insert(0, { DATA }); //Readd the original command byte
			if (isRemote(target)) { //Target is remote
				rest.insert(4, string({target.high, target.low})); //Insert target for routing
			}
			bool found = sendTo(target, rest); //Send modified data to target
			/*
			 * Forwarded response to data send request
			 * CMD DATA (3)
			 * GERTADDR (4 bytes, numerical)
			 * DATA (Variable length up to 247 bytes, ASCII format)
			 */
			if (found)
				sendTo(gate, string({ STATE, SENT }));
				/*
				 * Response to successful data send request.
				 * CMD STATE (0)
				 * STATE SENT (5)
				 */
			else
				sendTo(gate, string({ STATE, FAILURE, NO_ROUTE }));
				/*
				 * Response to failed data send request.
				 * CMD STATE (0)
				 * STATE FAILURE (0)
				 * REASON NO_ROUTE (3)
				 */
			return;
		}
		case QUERY: {
			sendTo(gate, string({ STATE, gate->state }));
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
			 * STATE CLOSED (4)
			 */
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
	USHORT* restShort = (USHORT*)rest.c_str();
	switch (command) {
		case ROUTE: {
			GERTaddr target = {restShort[2], restShort[3]};
			GERTaddr source = {restShort[0], restShort[1]};
			string cmd = { DATA };
			cmd += source.high;
			cmd += source.low;
			cmd += rest.erase(0, 8);
			sendTo(target, cmd);
			return;
		}
		case REGISTERED: {
			GERTaddr target = {restShort[0], restShort[1]};
			setRoute(target, geds);
			return;
		}
		case UNREGISTERED: {
			GERTaddr target = {restShort[0], restShort[1]};
			removeRoute(target);
			return;
		}
		case RESOLVE: {
			GERTaddr target = {restShort[0], restShort[1]};
			rest.erase(0, 4);
			GERTkey key = rest;
			addResolution(target, key);
			return;
		}
		case UNRESOLVE: {
			GERTaddr target = {restShort[0], restShort[1]};
			removeResolution(target);
			return;
		}
		case LINK: {
			char* restRaw = (char*)rest.c_str();
			UCHAR ipaddr[4] = {restRaw[0], restRaw[1], restRaw[2], restRaw[3]};
			ipAddr target(ipaddr);
			rest.erase(0, 4);
			restShort = (USHORT*)rest.c_str();
			portComplex ports = {restShort[0], restShort[1]};
			addPeer(target, ports);
			return;
		}
		case UNLINK: {
			char* restRaw = (char*)rest.c_str();
			UCHAR ipaddr[4] = {restRaw[0], restRaw[1], restRaw[2], restRaw[3]};
			ipAddr target(ipaddr);
			removePeer(target);
			return;
		}
		case CLOSEPEER: {
			sendTo(geds, string({ CLOSEPEER }));
			closeTarget(geds);
			return;
		}
	}
}

DLLExport void killGateway(gateway* gate) {
	sendTo(gate, string({ CLOSE })); //SEND CLOSE REQUEST
	sendTo(gate, string({ STATE, CLOSED })); //SEND STATE UPDATE TO CLOSED (0, 4)
	closeTarget(gate);
}

DLLExport void killGEDS(peer* geds) {
	sendTo(geds, string({ CLOSEPEER })); //SEND CLOSE REQUEST
	closeTarget(geds);
}

ENDEXPORT
