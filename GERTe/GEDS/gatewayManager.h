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

gateway* getGate(GERTaddr);
bool sendTo(GERTaddr, string);
void initGate(void *);
void closeTarget(gateway*);
#endif
