#include "Key.h"
#include "peerManager.h"
#include "gatewayManager.h"
#include "logging.h"
#include <thread>
using namespace std;

typedef unsigned long long pointer;

extern volatile bool running;

enum scanResult {
	CLEAN,
	MINOR_ERR,
	MAJOR_ERR
};

int scanGateway(Gateway * checkgate, string addr) {
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
	return errs;
}

int emergencyScan() { //EMERGENCY CLEANUP FOR TERMINATE/ABORT/SIGNAL HANDLING
	bool peerErr = false;
	int total = 0;
	int errs = 0;
	debug("[ESCAN] Emergency scan triggered!");
	for (peerIter iter; !iter.isEnd(); iter++) {
		Peer * checkpeer = *iter;
		if (checkpeer == nullptr) {
			debug("[ESCAN] Found a missing peer within peers map");
			errs++;
			continue;
		}
		void * checksock = checkpeer->sock;
		if (checksock == nullptr) {
			debug("[ESCAN] Peer missing socket");
			errs++;
		}
	}
	debug("[ESCAN] Peer error count: " + to_string(errs));
	if (errs > 0)
		peerErr = true;
	total += errs;
	errs = 0;
	for (gatewayIter iter; !iter.isEnd(); iter++) {
		Gateway * checkgate = *iter;
		Address addrstrct = checkgate->addr;
		string addr = addrstrct.stringify();
		errs += scanGateway(checkgate, addr);
	}
	debug("[ESCAN] Gateway error count: " + to_string(errs));
	total += errs;
	errs = 0;
	for (noAddrIter iter; !iter.isEnd(); iter++) {
		Gateway * checkgate = *iter;
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
