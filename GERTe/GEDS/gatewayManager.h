#ifndef __GATEWAY_MANAGER__
#define __GATEWAY_MANAGER__
#include "Gateway.h"
#include <map>
#include <vector>

typedef map<Address, Gateway*>::iterator gatewayPtr;
typedef vector<Gateway*>::iterator noAddrPtr;

class gatewayIter {
	gatewayPtr ptr;
	public:
		bool isEnd();
		gatewayIter operator++(int);
		gatewayIter();
		Gateway* operator*();
		void erase();
};

class noAddrIter {
	noAddrPtr ptr;
	public:
		bool isEnd();
		noAddrIter operator++(int);
		noAddrIter();
		Gateway* operator*();
		void erase();
};

extern "C" {
	bool sendToGateway(Address, string);
	void initGate(void *);
	void gateWatcher();
	bool isLocal(Address);
}
#endif
