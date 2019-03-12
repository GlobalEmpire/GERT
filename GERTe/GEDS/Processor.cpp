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

void worker(void * thisref) {
	((Processor*)thisref)->run();
}

Processor::Processor(Poll * poll) : poll(poll) {
	proc = new std::thread{ &Processor::run, this };
}

Processor::~Processor() {
	poll->update();
	((std::thread*)proc)->join();
}

void Processor::run() {
	poll->claim();

	while (true) {
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
