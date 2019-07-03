#include "Peer.h"
#include "Gateway.h"
#include "logging.h"
#include <thread>
#include <map>
#include <vector>
using namespace std;

typedef unsigned long long pointer;

extern volatile bool running;
extern map<IP, Peer*> peers;
extern vector<UGateway*> noAddrList;
extern std::map<Address, Gateway*> gateways;

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
	for (std::pair<IP, Peer*> pair : peers) {
		Peer* checkpeer = pair.second;
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
	for (std::pair<Address, Gateway*> pair : gateways) {
		Gateway* checkgate = pair.second;
		Address addrstrct = checkgate->addr;
		string addr = addrstrct.stringify();
		errs += scanGateway(checkgate, addr);
	}
	debug("[ESCAN] Gateway error count: " + to_string(errs));
	total += errs;
	errs = 0;
	for (UGateway* gate : noAddrList) {
		errs += scanGateway(gate, "without address");
	}
	total += errs;
	debug("[ESCAN] Emergency scan finished with " + to_string(errs) + " errors\n");
	if (total == 0)
		return CLEAN;
	else if (peerErr == false)
		return MINOR_ERR;
	return MAJOR_ERR;
}
