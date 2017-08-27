#pragma once
#include "Versioning.h"

class Gateway;
class Peer;

class Version {
public:
	bool(*procGate)(Gateway*);
	bool(*procPeer)(Peer*);
	void(*killGate)(Gateway*);
	void(*killPeer)(Peer*);
	Versioning vers;
	void* handle;
};
