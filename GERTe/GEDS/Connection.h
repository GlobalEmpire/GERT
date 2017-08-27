#pragma once
#include "Version.h"

class Connection {
public:
	void * sock;
	Version * api;
	unsigned char state;
	Connection(void * socket, Version * vers = nullptr) : sock(socket), api(vers) {};
};
