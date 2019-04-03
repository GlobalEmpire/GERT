#pragma once
#include "netty.h"
#include <vector>

struct Event_Data {
	SOCKET fd;
	Connection * ptr;
};

class Poll {

	friend void removeTracker(SOCKET, Poll*);
	friend void cleanup();

	void * handler = nullptr;

#ifndef _WIN32
	int efd;
	std::vector<Event_Data*> tracker;
#else
	std::vector<void*> tracker;
	std::vector<void*> events;
#endif

public:
	Poll();
	~Poll();

	void add(SOCKET, Connection* = nullptr);
	void remove(SOCKET);
	Event_Data wait();
	void claim();
	void update();
};
