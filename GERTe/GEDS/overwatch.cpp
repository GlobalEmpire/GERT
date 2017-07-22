#include "keyDef.h"
#include "peerManager.h"
#include "gatewayManager.h"

typedef unsigned long long pointer;

extern bool running;

enum scanResult {
	CLEAN,
	MINOR_ERR,
	MAJOR_ERR
};

int scanAPI(version * checkapi, string addr, string type) {
	int errs = 0;
	if (checkapi == nullptr) {
		debug("[ESCAN] " + type + " " + addr + " missing api");
		return 1;
	}
	string apiopeer = "[ESCAN] " + type + " " + addr + " API missing ";
	if (checkapi->killGate == nullptr) {
		debug(apiopeer + "killGate function");
		errs++;
	}
	if (checkapi->killPeer == nullptr) {
		debug(apiopeer + "killPeer function");
		errs++;
	}
	if (checkapi->procGate == nullptr) {
		debug(apiopeer + "procGate function");
		errs++;
	}
	if (checkapi->procPeer == nullptr) {
		debug(apiopeer + "procPeer function");
		errs++;
	}
	return errs;
}

int scanGateway(gateway * checkgate, string addr) {
	int errs = 0;
	if (checkgate == nullptr) {
		debug("[ESCAN] Gateway " + addr + " missing from gateways map");
		return 1;
	}
	void * checksock = checkgate->sock;
	if (checksock == nullptr) {
		debug("[ESCAN] Gateway " + addr + " missing socket");
		errs++;
	}
	version * checkapi = checkgate->api;
	errs += scanAPI(checkapi, addr, "Gateway");
	return errs;
}

int emergencyScan() { //EMERGENCY CLEANUP FOR TERMINATE/ABORT/SIGNAL HANDLING
	bool peerErr = false;
	int total = 0;
	int errs = 0;
	debug("[ESCAN] Emergency scan triggered!");
	for (peerIter iter; !iter.isEnd(); iter++) {
		peer * checkpeer = *iter;
		if (checkpeer == nullptr) {
			debug("[ESCAN] Found a missing peer within peers map");
			errs++;
			continue;
		}
		string addr = checkpeer->addr.stringify();
		void * checksock = checkpeer->sock;
		if (checksock == nullptr) {
			debug("[ESCAN] Peer " + addr + "missing socket");
			errs++;
		}
		version * checkapi = checkpeer->api;
		errs += scanAPI(checkapi, addr, "Peer");
	}
	debug("[ESCAN] Peer error count: " + to_string(errs));
	if (errs > 0)
		peerErr = true;
	total += errs;
	errs = 0;
	for (gatewayIter iter; !iter.isEnd(); iter++) {
		gateway * checkgate = *iter;
		GERTaddr addrstrct = checkgate->addr;
		string addr = addrstrct.stringify();
		errs += scanGateway(checkgate, addr);
	}
	debug("[ESCAN] Gateway error count: " + to_string(errs));
	total += errs;
	errs = 0;
	for (noAddrIter iter; !iter.isEnd(); iter++) {
		gateway * checkgate = *iter;
		errs += scanGateway(checkgate, "without address");
	}
	total += errs;
	debug("[ESCAN] Emergency scan finished with " + to_string(errs) + " errors\n");
	if (total == 0)
		return CLEAN;
	else if (peerErr == false)
		return MINOR_ERR;
	return MAJOR_ERR;
}

void overwatch() {
	while (running) {
		gateWatcher();
		peerWatcher();
	}
}
