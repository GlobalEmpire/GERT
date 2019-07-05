#pragma once
#include "netty.h"
#include <vector>

class Poll {
#ifndef _WIN32
	int efd;
#else
	std::vector<void*> events;

	inline INet* WSALoop();
#endif

	std::vector<INet*> tracker;

public:
	Poll();
	~Poll();

	void add(INet*);
	void remove(INet*);
	INet* wait();
};
