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
		void process();
		void kill();
		static Gateway* lookup(Address);
		void transmit(std::string);
		bool assign(Address, Key);
		void close();
};
