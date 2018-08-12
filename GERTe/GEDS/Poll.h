#pragma once
#include "netty.h"

struct Event_Data {
	int fd;
	void * ptr;
};

class Poll {
	int efd;
	void * tracker;

public:
	Poll();
	~Poll();

	void add(SOCKET, void*);
	void remove(SOCKET);
	Event_Data wait();
};
