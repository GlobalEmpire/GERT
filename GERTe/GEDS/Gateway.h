#pragma once
#include "UGateway.h"
#include "Address.h"

//THIS IS NOT EVEN MY FINAL FORM!!!

class Gateway : public UGateway {
public:
	Address addr;

	Gateway(Address, UGateway*);
	~Gateway();

	static Gateway * lookup(Address);
	static bool sendTo(Address, std::string);

	void process();

	void closeTunnel(uint16_t);
};

