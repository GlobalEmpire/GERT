#pragma once
#include "Connection.h"
#include "Address.h"
#include "Key.h"
#include "Ports.h"

class Gateway : public Connection {
	Gateway(void*);

	friend void runServer(void*, void*);

	public:
		Address addr;
		bool local = false;
		void process() { api->procGate(this); };
		void kill() { api->killGate(this); };
		static Gateway* lookup(Address);
		//~Gateway();
		void transmit(string);
		bool assign(Address, Key);
		void close();
};
