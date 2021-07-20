#include "Poll.h"
#include "../Util/logging.h"
#include "../Util/Error.h"

#ifndef WIN32
#include <sys/epoll.h>
#include <unistd.h>
#include <csignal>
#else
#include <WinSock2.h>
#include <stdexcept>
#endif

extern volatile bool running;

#ifdef WIN32
#define GETPTR store->data
struct INNER {
	WSAEVENT event;
	Socket* data;
};
#else
#define GETPTR store
#endif

void Poll::removeTracker(Socket* ptr) {
	int i = 0;

#ifdef WIN32
	std::lock_guard guard{ lock };
#endif

	for (auto iter = tracker.begin(); iter != tracker.end(); iter++)
	{
	    auto* store = *iter;

		if (GETPTR == ptr) {
#ifdef WIN32
			auto eiter = events.begin() + i;
			WSACloseEvent(*eiter);
			events.erase(eiter);
			events.shrink_to_fit();

			delete store->data;
#endif
			delete store;

			tracker.erase(iter);
			tracker.shrink_to_fit();

			return;
		}

		i++;
	}
}

#ifndef WIN32
Poll::Poll() {
	efd = epoll_create(1);
}
#endif

Poll::~Poll() {
#ifndef WIN32
	close(efd);

    for (auto* ptr: tracker)
        delete ptr;
#else
	for (INNER* data : tracker) {
		auto * store = data;

		delete store->data;
		WSACloseEvent(store->event);

		delete store;
	}
#endif
}

void Poll::add(Socket* ptr) { //Adds the file descriptor to the pollset and tracks useful information
#ifndef WIN32
	epoll_event newEvent{ EPOLLIN | EPOLLONESHOT, ptr };

	if (epoll_ctl(efd, EPOLL_CTL_ADD, ptr->sock, &newEvent) == -1)
        throw std::system_error();

	tracker.push_back(ptr);
#else
	WSAEVENT e = WSACreateEvent();
	WSAEventSelect(ptr->sock, e, FD_READ | FD_ACCEPT | FD_CLOSE);

    auto * store = new INNER{
            e,
            ptr
    };

	std::lock_guard guard{ lock };

	events.push_back(e);
	tracker.push_back(store);

	fired.erase(ptr);
#endif
}

void Poll::remove(Socket* ptr) {
#ifndef WIN32
	if (epoll_ctl(efd, EPOLL_CTL_DEL, ptr->sock, nullptr) == -1 && errno != ENOENT)
		throw std::system_error();
#endif

	removeTracker(ptr);
}

Socket* Poll::wait() { //Awaits for an event on a file descriptor.
#ifndef WIN32
	epoll_event eEvent{0, nullptr};

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);

	if (epoll_pwait(efd, &eEvent, 1, -1, &mask) == -1) {
		if (errno == EINTR)
		    return nullptr;
		else
		    throw std::system_error();
	}

	return (Socket*)eEvent.data.ptr;
#else
	while (true) {
		if (events.empty())
			SleepEx(INFINITE, true); //To prevent WSA crashes, if no sockets are in the poll, wait indefinitely until awoken
		else {
		    std::vector<INNER*> trackcopy;
		    std::vector<void*> eventcopy;

		    // Make copies to prevent modification from screwing with us.
            {
                std::lock_guard guard{ lock };

                trackcopy = tracker;
                eventcopy = events;
            }

			DWORD result = WSAWaitForMultipleEvents(eventcopy.size(), eventcopy.data(), false, WSA_INFINITE, true); //Interruptable select

			if (result == WSA_WAIT_FAILED)
				socketError("Socket poll error: ");
			else if (result != WSA_WAIT_IO_COMPLETION && result != WSA_WAIT_TIMEOUT) {
			    std::lock_guard guard{ lock };

				DWORD offset = result - WSA_WAIT_EVENT_0;

				if (offset < eventcopy.size()) {
					auto * store = trackcopy[offset];

                    if (fired.count(store->data))
                        continue;

					WSANETWORKEVENTS triggered;
					int res = WSAEnumNetworkEvents(store->data->sock, events[offset], &triggered);
					if (res == SOCKET_ERROR) {
						socketError("Failed to reset poll event: ");
						throw std::runtime_error{ "Failed to reset pull event" };
					}

					if (triggered.lNetworkEvents != 0) { // Ignore spurious wakeups
					    fired.insert({store->data, store});

					    size_t c = std::erase(tracker, store);
					    std::erase(events, store->event);

					    if (c == 0)
					        throw std::runtime_error{ "" };

                        return store->data;
                    }
				}
				else
					error("Attempted to return from wait, but result was invalid: " + std::to_string(result));
			}
		}

		if (!running)
			return nullptr;
	}
#endif
}

void Poll::clear(Socket* orig) {
    if (!orig->valid())
        return;

#ifdef WIN32
    std::lock_guard guard{ lock };
    if (fired.count(orig)) {
        auto* store = fired[orig];

        WSAResetEvent(store->event);

        tracker.push_back(store);
        events.push_back(store->event);

        fired.erase(orig);
    }
#else
    epoll_event event{ EPOLLIN | EPOLLONESHOT, orig };
    if (epoll_ctl(efd, EPOLL_CTL_MOD, orig->sock, &event) == -1 && errno != ENOENT)
        throw std::system_error();
#endif
}
