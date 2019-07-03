#include "Address.h"
#include "Peer.h"

//THIS IS NOT EVEN MY FINAL FORM!!!

class RGateway {
	friend Peer;

	Peer * relay;

public:
	Address addr;

	RGateway(Address, Peer*);
	~RGateway();

	static RGateway * lookup(Address);
	static bool sendTo(Address, std::string);
	static void clean(Peer*);

	void transmit(std::string);
};