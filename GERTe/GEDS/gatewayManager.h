#pragma once
#include "Gateway.h"
#include <map>
#include <vector>

typedef std::map<Address, Gateway*>::iterator gatewayPtr;
typedef std::vector<Gateway*>::iterator noAddrPtr;

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

bool sendToGateway(Address, std::string);
void gateWatcher();
bool isLocal(Address);
