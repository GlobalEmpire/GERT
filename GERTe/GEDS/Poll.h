#pragma once
#include "netty.h"

#ifdef _WIN32
#include <map>
#include <mutex>
#else
#include <vector>
#endif

struct Event_Data {
	SOCKET fd;
	void * ptr;
};

class Poll {
#ifndef _WIN32
	int efd;
	std::vector<Event_Data*> tracker;
#else
	std::map<SOCKET, Event_Data*> socks;
	std::mutex viewing;
#endif

public:
	Poll();
	~Poll();

	void add(SOCKET, void* = nullptr);
	void remove(SOCKET);
	Event_Data wait();
};
