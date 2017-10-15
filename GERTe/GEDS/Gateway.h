#pragma once
#include "Connection.h"
#include "Address.h"
#include "Key.h"
#include "Ports.h"

class Gateway : public Connection {
	public:
		Address addr;
		bool local = false;
		void process() { api->procGate(this); };
		void kill() { api->killGate(this); };
		Gateway(void*);
		Gateway(Address);
		void transmit(string);
		bool assign(Address, Key);
		void close();
};
