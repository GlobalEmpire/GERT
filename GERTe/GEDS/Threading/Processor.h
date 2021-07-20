#pragma once

#include "../Networking/Socket.h"
#include "Poll.h"
#include <atomic>

namespace std {
    class thread;
}

class Processor {
	Poll * poll;

	std::atomic<bool> running = true;

	unsigned int numThreads;
	std::thread* threads;

	void run();
public:
	explicit Processor(Poll *);
	~Processor();

    void add(Socket*);
    void remove(Socket*);

	void update();
};
