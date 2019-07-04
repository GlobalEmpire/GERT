#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
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
		Event_Data data = poll->wait();

		if (data.fd == 0)
			return;

		SOCKET sock = data.fd;
		INet * obj = data.ptr;

		char test[1];
		if (obj->type == INet::Type::CONNECT && recv(sock, test, 1, MSG_PEEK) != 1) { //If there's no data when we were told there was, the socket closed
			delete obj;
		}
		else
			obj->process();
	}
}

void Processor::update() {
	std::thread* threadp = (std::thread*)pool;

	for (int i = 0; i < poolsize; i++)
		interrupt(threadp[i]);
}
