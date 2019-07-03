#include "Pool.h"
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <signal.h>
#endif

#ifdef _WIN32
void apc(ULONG_PTR s) {}
#endif

void inline interrupt(std::thread &thread) {
#ifdef _WIN32
	QueueUserAPC(apc, thread.native_handle(), 0);
#else
	pthread_kill(thread.native_handle(), SIGUSR1);
#endif
}

Pool::Pool(POOLFUNC func, void * data) {
	threads = std::thread::hardware_concurrency();

	if (threads == 0)
		threads = 1;

	std::thread * threadp = new std::thread[threads];

	for (int i = 0; i < threads-1; i++) {
		threadp[i] = std::thread{ func, data };
	}

	arr = threadp;
}

Pool::~Pool() {
	std::thread * threadp = (std::thread*)arr;

	for (int i = 0; i < threads; i++) {
		::interrupt(threadp[i]);
		threadp[i].join();
	}
}

void Pool::interrupt() {
	std::thread * threadp = (std::thread*)arr;

	for (int i = 0; i < threads; i++) {
		::interrupt(threadp[i]);
	}
}
