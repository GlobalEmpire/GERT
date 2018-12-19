#pragma once
#include "Versioning.h"

class Gateway;
class Peer;

class Version {
public:
	void(*procGate)(Gateway*);
	void(*procPeer)(Peer*);
	void(*killGate)(Gateway*);
	void(*killPeer)(Peer*);
	Versioning vers;
	void* handle;
};
