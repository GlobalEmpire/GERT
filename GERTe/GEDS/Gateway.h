#pragma once
#include "Address.h"
#include "Key.h"
#include "Ports.h"

class Gateway : public Connection {
	Gateway(void*);

	friend void runServer();

	public:
		Address addr;
		bool local = false;
		static Gateway* lookup(Address);
		void transmit(std::string);
		bool assign(Address, Key);
		void close(bool=false);
};
