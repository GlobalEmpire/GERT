#pragma once
#include "../Networking/netty.h"
#include <vector>

struct Event_Data {
	SOCKET fd;
	Connection * ptr;
};

class Poll {
	friend void removeTracker(SOCKET, Poll*);
	friend void cleanup();

	int len = 0;
	void* threads = nullptr;

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
	void claim(void*, int);
	void update();
};
