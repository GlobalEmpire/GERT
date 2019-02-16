#pragma once
#include "Key.h"
#include "Ports.h"

class Gateway : public Connection {
	Gateway(SOCKET*);

	friend void runServer();

	public:
		~Gateway();
		Address addr;
		bool local = false;
		static Gateway* lookup(Address);
		void transmit(std::string);
		bool assign(Address, Key);
		void close();
		void process();
};
