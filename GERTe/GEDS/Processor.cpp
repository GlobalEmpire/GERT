#ifdef _WIN32
#include <Windows.h>
#else
#include <signal.h>
#endif

#include "Processor.h"
#include "Poll.h"
#include <thread>

#ifdef _WIN32
void apc([[maybe_unused]]ULONG_PTR s) {}
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

	pool = new std::thread[poolsize];

	for (unsigned int i = 0; i < poolsize; i++)
		pool[i] = std::thread{ &Poll::wait, poll };
}

Processor::~Processor() {
	for (unsigned int i = 0; i < poolsize; i++) {
		interrupt(pool[i]);
		pool[i].join();
	}
	delete pool;
}

void Processor::update() {
	for (unsigned int i = 0; i < poolsize; i++)
		interrupt(pool[i]);
}
