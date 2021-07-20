#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")

void apc([[maybe_unused]] ULONG_PTR _) {}
#else
#include <sys/socket.h>
#include <csignal>
#endif

#include "Processor.h"
#include "../Util/logging.h"
#include <thread>

Processor::Processor(Poll * poll) : poll(poll) {
    numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 1;

    threads = new std::thread[numThreads];

    for (int i = 0; i < numThreads; i++)
        threads[i] = std::thread{ &Processor::run, this };
}

Processor::~Processor() {
    running = false;

    update();

    for (int i = 0; i < numThreads; i++)
        threads[i].join();

    delete[] threads;
}

void Processor::run() {
	while (running) {
		Socket* data = poll->wait();
		if (data == nullptr)
            continue;

		if (data->valid())
            data->process();

		if (!data->valid()) { // Done seperately (not as an else) so that a loss of validity after process is detected
            remove(data);
            delete data;
		} else {
            poll->clear(data);
            update();
        }
	}
}

void Processor::add(Socket* ptr) { //Adds the file descriptor to the pollset and tracks useful information
    poll->add(ptr);
    update();
}

void Processor::remove(Socket* ptr) {
    poll->remove(ptr);
    update();
}

void Processor::update() {
    for (int i = 0; i < numThreads; i++) {
        auto& thread = threads[i];

        if (thread.get_id() == std::this_thread::get_id())
            continue;

#ifdef _WIN32
        QueueUserAPC(apc, thread.native_handle(), 0);
#else
        pthread_kill(thread.native_handle(), SIGUSR1);
#endif
    }
}
