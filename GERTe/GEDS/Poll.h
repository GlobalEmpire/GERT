#pragma once
#include "netty.h"
#include <vector>

struct Event_Data {
	SOCKET fd;
	void * ptr;
};

class Poll {

	friend void removeTracker(SOCKET, Poll*);
	friend void cleanup();

#ifndef _WIN32
	int efd;
	std::vector<Event_Data*> tracker;
#else
	std::vector<void*> tracker;
	std::vector<void*> events;
	void * handler = nullptr;
#endif

public:
	Poll();
	~Poll();

	void add(SOCKET, void* = nullptr);
	void remove(SOCKET);
	Event_Data wait();
	void claim();
};

#ifdef _WIN32
void apc(unsigned long long);
#endif
