#pragma once
#include "netty.h"

struct Event_Data {
	SOCKET fd;
	void * ptr;
};

class Poll {
#ifndef _WIN32
	int efd;
#else
#endif

	void * tracker;

public:
	Poll();
	~Poll();

	void add(SOCKET, void* = nullptr);
	void remove(SOCKET);
	Event_Data wait();
};
