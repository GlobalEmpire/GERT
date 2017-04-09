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

#include "GEDS Server.h"
using namespace std;

typedef unsigned char UCHAR;

enum gatewayCommands{
	STATE,
	REGISTER,
	DATA,
	QUERY,
	CLOSE
};

enum gatewayStates {
	FAILURE,
	CONNECTED,
	REGISTERED,
	DENIED,
	CLOSED
};

enum gatewayErrors {
	VERSION,
	BAD_KEY,
	ALREADY_REGISTERED
};

enum gedsCommands {
	ROUTE,
	REGISTERED,
	UNREGISTERED,
	RESOLVE,
	UNRESOLVE,
	LINK,
	UNLINK,
	CLOSE
};

DLLExport UCHAR major = 1;
DLLExport UCHAR minor = 0;
DLLExport UCHAR patch = 0;

DLLExport void processGateway(connection gateway, string packet) {
	if (gateway.state == FAILURE) {
		gateway.state = CONNECTED;
		sendTo(gateway.addr, string({ STATE, CONNECTED, (char)major, (char)minor, (char)patch }));
		return;
	}
	UCHAR command = packet.data[0];
	string rest = packet.erase(0, 1);
	switch (command) {
	case REGISTER:
		if (gateway.addr.high == 0 && gateway.addr.low == 0) {
			sendTo(gateway.addr, string({ STATE, FAILURE, ALREADY_REGISTERED }));
			return;
		}
		GERTaddr request = rest.data[0];
		rest.erase(0, 4);
		GERTkey requestkey = rest.data[0];
	case DATA:
		GERTaddr target = rest.data[0]; //Assign target address as first 4 bytes
		(GERTaddr)rest.data[0] = gateway.addr; //Set first 4 bytes to source address
		rest.insert(0, { DATA }); //Readd the original command byte
		sendTo(target, rest); //Send modified data to target
	case QUERY:
		sendTo(gateway.addr, string({ STATE, (char)gateway.state }));
	case CLOSE:
		sendTo(gateway.addr, string({ STATE, CLOSED }));
		closeConnection(gateway);
	}
}

DLLExport void processGEDS(connection geds, string packet) {
	if (geds.state == 0) {
		geds.state = 1;
		sendTo(geds, string({ (char)major, (char)minor, (char)patch });
		return;
	}
	UCHAR command = packet.data[0];
	string rest = packet.erase(0, 1);
	switch (command) {
		case ROUTE:
			GERTaddr target = rest.data[0];
			GERTaddr source = rest.data[0];
			string cmd = { DATA };
			cmd += (char)(source.high >> 8);
			cmd += (char)source.high;
			cmd += (char)(source.low >> 8);
			cmd += (char)source.low;
			cmd += rest.erase(0, 8);
			sendTo(target, cmd);
		case REGISTERED:
			GERTaddr target = rest.data[0];
			setRoute(target, geds);
		case UNREGISTERED:
			GERTaddr target = rest.data[0];
			removeRoute(target);
		case RESOLVE:
			GERTaddr target = rest.data[0];
			GERTkey key = rest.erase(0, 4).data[0];
			addResolution(target, key);
		case UNRESOLVE:
			GERTaddr target = rest.data[0];
			removeResolution(target);
		case LINK:
			addPeer(rest.data);
		case UNLINK:
			removePeer(rest.data);
		case CLOSE:
			sendTo(geds, string({ CLOSE }));
			closeConnection(geds);
	}
}

DLLExport void killGateway(connection gateway) {
	sendTo(gateway.addr, string({ CLOSE }));
	sendTo(gateway.addr, string({ STATE, CLOSED }));
	closeConnection(gateway);
}

DLLExport void killGEDS(connection geds) {
	sendTo(geds, string({ CLOSE }));
	closeConnection(geds);
}