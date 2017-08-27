#include "Versioning.h"

class Gateway;
class Peer;

class Version {
public:
	bool(*procGate)(Gateway*, string);
	bool(*procPeer)(Peer*, string);
	void(*killGate)(Gateway*);
	void(*killPeer)(Peer*);
	Versioning vers;
	void* handle;
};
