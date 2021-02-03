#include "../Peer/Peer.h"
#include "../Gateway/gatewayManager.h"
#include "logging.h"
#include <thread>
using namespace std;

typedef unsigned long long pointer;

extern volatile bool running;
extern map<IP, Peer*> peers;

enum scanResult {
	CLEAN,
	MINOR_ERR,
	MAJOR_ERR
};

int scanGateway(UGateway * checkgate, string addr) {
	int errs = 0;
	if (checkgate == nullptr) {
		debug("[ESCAN] Gateway " + addr + " missing from gateways map");
		return 1;
	}
	return errs;
}

int emergencyScan() { //EMERGENCY CLEANUP FOR TERMINATE/ABORT/SIGNAL HANDLING
	bool peerErr = false;
	int total = 0;
	int errs = 0;
	debug("[ESCAN] Emergency scan triggered!");
	for (map<IP, Peer*>::iterator iter = peers.begin(); iter != peers.end(); iter++) {
		Peer * checkpeer = iter->second;
		if (checkpeer == nullptr) {
			debug("[ESCAN] Found a missing peer within peers map");
			errs++;
			continue;
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
		UGateway * checkgate = *iter;
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
