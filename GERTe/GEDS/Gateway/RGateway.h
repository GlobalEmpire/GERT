#include "Address.h"
#include "../Peer/Peer.h"

class RGateway {
	friend void killAssociated(Peer*);

	Peer * relay;

public:
	Address addr;

	RGateway(Address, Peer*);
	~RGateway();

	static RGateway * lookup(Address);
	static bool sendTo(Address, std::string);

	void transmit(std::string);
};