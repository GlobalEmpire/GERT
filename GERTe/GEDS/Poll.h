#pragma once
#include "netty.h"
#include <vector>

struct Event_Data {
	SOCKET fd;
	INet * ptr;
};

class Poll {
	friend void removeTracker(SOCKET, Poll*);
	friend void cleanup();

#ifndef _WIN32
	int efd;
#else
	std::vector<void*> events;
#endif

	std::vector<Event_Data*> tracker;

public:
	Poll();
	~Poll();

	void add(SOCKET, INet*);
	void remove(SOCKET);
	Event_Data wait();
};
