#pragma once
#include "netDefs.h"
#include "Connection.h"
#include "Address.h"
#include "Key.h"

class Gateway : public Connection {
	public:
		Address addr;
		void process() { api->procGate(this); };
		void kill() { api->killGate(this); };
		Gateway(void* socket);
		void transmit(string);
		bool assign(Address, Key);
		void close();
};
