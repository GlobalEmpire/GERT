#pragma once
#include "netty.h"
#include <vector>

class Poll {
#ifndef _WIN32
	int efd;

	inline INet* linuxLoop();
#else
	std::vector<void*> events;

	inline INet* WSALoop();
#endif

	std::vector<INet*> tracker;

public:
	Poll();
	~Poll();

	void add(INet*);
	void remove(SOCKET);
	void wait();
};
