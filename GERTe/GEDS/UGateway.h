#pragma once
#include "Connection.h"
#include "Key.h"

class Gateway;

//THIS IS NOT EVEN MY FINAL FORM!!!

class UGateway : public Connection { //The OG Gateway
	UGateway(SOCKET);

	friend void runServer();

protected:
	UGateway(UGateway&&);

public:
	virtual ~UGateway();

	void transmit(std::string);
	bool assign(Address, Key);
	void close();
	void process(Gateway *);
	void process() { process(nullptr); };
};
