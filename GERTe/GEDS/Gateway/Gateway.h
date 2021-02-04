#pragma once
#include "../Networking/Connection.h"
#include "Key.h"
#include "GERTc.h"

class Gateway : public Connection {
	Gateway(SOCKET);

	friend void runServer();

	GERTc addr;

public:
	virtual ~Gateway();

	void transmit(std::string);
	bool assign(Address, Key);
	void close();
	void process();

    static Gateway * lookup(Address);
    static bool sendTo(Address, const std::string&);
};
