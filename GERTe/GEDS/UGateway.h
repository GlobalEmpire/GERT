#pragma once
#include "Connection.h"
#include "Key.h"
#include "Tunnel.h"
#include <map>

class Gateway;

//THIS IS NOT EVEN MY FINAL FORM!!!

class UGateway : public Connection { //The OG Gateway

	UGateway(SOCKET);

	friend void runServer();

protected:
	UGateway(UGateway&&);

public:
	static std::map<uint16_t, Tunnel> tunnels;

	virtual ~UGateway();

	void transmit(std::string);
	bool assign(Address, Key);
	void close();
	void process(Gateway *);
	void process() { process(nullptr); };
};
