#pragma once
#include "netty.h"
#include <vector>

struct Event_Data {
	SOCKET fd;
	void * ptr;
};

class Poll {

	friend void removeTracker(SOCKET, Poll*);

#ifndef _WIN32
	int efd;
	std::vector<Event_Data*> tracker;
#else
	std::vector<void*> tracker;
	std::vector<void*> events;
	void * handler;
#endif

public:
	Poll();
	~Poll();

	void add(SOCKET, void* = nullptr);
	void remove(SOCKET);
	Event_Data wait();
};