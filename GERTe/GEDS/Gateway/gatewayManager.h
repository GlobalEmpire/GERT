#pragma once
#include <map>
#include <vector>
#include "Gateway.h"

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
