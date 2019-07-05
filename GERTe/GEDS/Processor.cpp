#ifdef _WIN32
#include <Windows.h>
#else
#include <signal.h>
#endif

#include "Processor.h"
#include "Poll.h"
#include <type_traits>
#include <thread>

extern volatile bool running;

#ifdef _WIN32
void apc(ULONG_PTR s) {}
#endif

void inline interrupt(std::thread& thread) {
#ifdef _WIN32
	QueueUserAPC(apc, thread.native_handle(), 0);
#else
	pthread_kill(thread.native_handle(), SIGUSR1);
#endif
}

Processor::Processor(Poll * poll) : poll(poll) {
	poolsize = std::thread::hardware_concurrency();

	if (poolsize == 0)
		poolsize = 1;

	std::thread* threadp = new std::thread[poolsize];

	for (int i = 0; i < poolsize; i++)
		threadp[i] = std::thread{ &Processor::run, this };

	pool = threadp;
}

Processor::~Processor() {
	std::thread* threadp = (std::thread*)pool;

	for (int i = 0; i < poolsize; i++) {
		interrupt(threadp[i]);
		threadp[i].join();
	}
	delete threadp;
}

void Processor::run() {
	while (true) {
		INet* obj = poll->wait();

		if (obj == nullptr)
			return;

		obj->process();
	}
}

void Processor::update() {
	std::thread* threadp = (std::thread*)pool;

	for (int i = 0; i < poolsize; i++)
		interrupt(threadp[i]);
}
