#include "Address.h"
#include "Peer.h"

//THIS IS NOT EVEN MY FINAL FORM!!!

class RGateway {
	friend void killAssociated(Peer*);
public:
	Peer* relay;
	Address addr;

	RGateway(Address, Peer*);
	~RGateway();

	static RGateway * lookup(Address);
	static bool sendTo(Address, std::string);

	void transmit(std::string);
};