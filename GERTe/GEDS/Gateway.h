#pragma once
#include "netDefs.h"
#include "Key.h"

class Gateway : public connection {
	public:
		Address addr;
		void process(string data) { api->procGate(this, data); };
		void kill() { api->killGate(this); };
		Gateway(void* socket);
		void transmit(string);
		bool assign(Address, Key);
		void close();
};
