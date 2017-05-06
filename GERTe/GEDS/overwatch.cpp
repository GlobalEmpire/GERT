#include "libDefs.h"
#include "keyDef.h"
#include "netDefs.h"
#include <map>
#include <forward_list>
#include "logging.h"

typedef unsigned long long pointer;

typedef map<ipAddr, peer*>::iterator peersIter;
typedef map<GERTaddr, gateway*>::iterator gatewaysIter;

typedef forward_list<gateway*>::iterator noAddrIter;

//BEGIN VARIABLE HOOKS
extern map<ipAddr, peer*> peers;
extern map<GERTaddr, gateway*> gateways;

extern forward_list<gateway*> noAddr;
//END VARIABLE HOOKS

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
	int errs = 0;
	debug("[ESCAN] Emergency scan triggered!");
	for (peersIter iter = peers.begin(); iter != peers.end(); iter++) {
		peer * checkpeer = iter->second;
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
	for (gatewaysIter iter = gateways.begin(); iter != gateways.end(); iter++) {
		gateway * checkgate = iter->second;
		GERTaddr addrstrct = iter->first;
		string addr = addrstrct.stringify();
		errs += scanGateway(checkgate, addr);
	}
	for (noAddrIter iter = noAddr.begin(); iter != noAddr.end(); iter++) {
		gateway * checkgate = *iter;
		errs += scanGateway(checkgate, "without address");
	}
	debug("[ESCAN] Emergency scan finished with " + to_string(errs) + " errors\n");
	return errs;
}

/*
void overwatch() {
	//Implement some kind of overwatch for pointers and garbage collecting them.
}*/
