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
#include "Pool.h"
#include "Poll.h"
#include <type_traits>

extern volatile bool running;

void worker(void * thisref) {		// Redirection, allows use of a member function
	((Processor*)thisref)->run();
}

Processor::Processor(Poll * poll) : poll(poll) {
	pool = new Pool{ worker, this };
}

Processor::~Processor() {
	Pool * poolp = (Pool*)pool;

	poolp->interrupt();
	delete poolp;
}

void Processor::run() {
	while (true) {
		Event_Data data = poll->wait();

		if (data.fd == 0)
			return;

		SOCKET sock = data.fd;
		INet * obj = data.ptr;

		char test[1];
		if (obj->type == INet::Type::CONNECT && recv(sock, test, 1, MSG_PEEK) == 0) { //If there's no data when we were told there was, the socket closed
			poll->remove(data.fd);
			delete obj;
		}
		else
			obj->process();
	}
}

void Processor::update() {
	((Pool*)pool)->interrupt();
}
