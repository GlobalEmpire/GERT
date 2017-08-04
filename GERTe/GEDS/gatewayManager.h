#ifndef __GATEWAY_MANAGER__
#define __GATEWAY_MANAGER__
#include "netDefs.h"
#include <map>
#include <vector>

typedef map<GERTaddr, gateway*>::iterator gatewayPtr;
typedef vector<gateway*>::iterator noAddrPtr;

class gatewayIter {
	gatewayPtr ptr;
	public:
		bool isEnd();
		gatewayIter operator++(int);
		gatewayIter();
		gateway* operator*();
};

class noAddrIter {
	noAddrPtr ptr;
	public:
		bool isEnd();
		noAddrIter operator++(int);
		noAddrIter();
		gateway* operator*();
		void erase();
};

extern "C" {
	gateway* getGate(GERTaddr);
	bool sendToGateway(GERTaddr, string);
	void initGate(void *);
	void closeGateway(gateway*);
	void gateWatcher();
	bool isLocal(GERTaddr);
}
#endif
