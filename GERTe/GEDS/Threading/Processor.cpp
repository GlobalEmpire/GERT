#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "Processor.h"
#include "Poll.h"
#include <type_traits>
#include <thread>

extern volatile bool running;

Processor::Processor(Poll * poll) : poll(poll) {
    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 1;

    threads = new std::thread[numThreads];

    for (int i = 0; i < numThreads; i++)
        ((std::thread*)threads)[i] = std::thread{ &Processor::run, this };

    poll->claim(threads, numThreads);
}

Processor::~Processor() {
	poll->update();

    for (int i = 0; i < numThreads; i++)
        ((std::thread*)threads)[i].join();

    delete[] (std::thread*)threads;
}

void Processor::run() {
	while (running) {
		Event_Data data = poll->wait();

		if (data.fd == 0)
			return;

		SOCKET sock = data.fd;
		Connection * conn = data.ptr;

		char test[1];
		if (recv(sock, test, 1, MSG_PEEK) == 0) { //If there's no data when we were told there was, the socket closed
			poll->remove(data.fd);
			delete conn;
		}
		else
			conn->process();
	}
}
