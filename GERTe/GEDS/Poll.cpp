#include "Poll.h"
#include "logging.h"
#include "Error.h"

#ifndef _WIN32
#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#else
#include <WinSock2.h>
#include <mutex>
#endif

extern volatile bool running;

#ifdef _WIN32
#define GETFD store->data->fd

struct INNER {
	WSAEVENT * event;
	Event_Data * data;
};

typedef std::vector<void*> TrackerT;

std::mutex lock;
std::vector<int> fired;
thread_local int last = INVALID_SOCKET;
#else
#define GETFD store->fd
typedef std::vector<Event_Data*> TrackerT;
thread_local Event_Data * last = nullptr;
#endif

void removeTracker(SOCKET fd, Poll* context) {
	TrackerT &tracker = context->tracker;

	int i = 0;
	for (TrackerT::iterator iter = tracker.begin(); iter != tracker.end(); iter++)
	{
#ifdef _WIN32
		INNER * store = (INNER*)(*iter);
#else
		Event_Data * store = *iter;
#endif

		if (GETFD == fd) {
#ifdef _WIN32
			std::vector<void*>::iterator eiter = context->events.begin() + i;

			lock.lock();
			for (std::vector<int>::iterator fiter = fired.begin(); fiter != fired.end(); iter++) {
				if (*fiter == GETFD) {
					fired.erase(fiter);
					break;
				}
			}
			lock.unlock();

			WSACloseEvent(*eiter);
			context->events.erase(eiter);
			context->events.shrink_to_fit();

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

#ifdef _WIN32
bool tryFire(int fd) {
	lock.lock();
	for (std::vector<int>::iterator iter = fired.begin(); iter != fired.end(); iter++) {
		if (*iter == fd) {
			lock.unlock();
			return false;
		}
	}
	
	fired.push_back(fd);
	lock.unlock();
	return true;
}
#endif

Poll::Poll() {
#ifndef _WIN32
	efd = epoll_create(1);
#endif
}

Poll::~Poll() {
	if (running)
		warn("NOTE: POLL HAS CLOSED. THIS IS A POTENTIAL MEMORY/RESOURCE LEAK. NOTIFY DEVELOPERS IF THIS OCCURS WHEN GEDS IS NOT CLOSING!");

#ifndef _WIN32
	close(efd);

	for (Event_Data * data : tracker)
	{
		delete data;
	}
#else
	for (void * data : tracker) {
		INNER * store = (INNER*)data;

		delete store->data;
		WSACloseEvent(store->event);

		delete store;
	}
#endif
}

void Poll::add(SOCKET fd, Connection * ptr) { //Adds the file descriptor to the pollset and tracks useful information
	Event_Data * data = new Event_Data{
		fd,
		ptr
	};

#ifndef _WIN32
	epoll_event newEvent;

	newEvent.events = EPOLLIN | EPOLLONESHOT;

	newEvent.data.ptr = data;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &newEvent) == -1)
		throw errno;

	tracker.push_back(data);
#else
	WSAEVENT e = WSACreateEvent();
	WSAEventSelect(fd, e, FD_READ | FD_ACCEPT | FD_CLOSE);

	events.push_back(e);

	INNER * store = new INNER{
		&(events.back()),
		data
	};

	tracker.push_back((void*)store);
#endif
}

void Poll::remove(SOCKET fd) {
#ifndef _WIN32
	if (last != nullptr && last->fd == fd)
		last = nullptr;
#else
	if (last == fd)
		last = INVALID_SOCKET;
#endif

	removeTracker(fd, this);
}

Event_Data Poll::wait() { //Awaits for an event on a file descriptor. Returns the Event_Data for a single event
#ifndef _WIN32
	if (last != nullptr) {
		epoll_event newEvent;
		newEvent.events = EPOLLIN | EPOLLONESHOT;
		newEvent.data.ptr = last;

		if (epoll_ctl(efd, EPOLL_CTL_MOD, last->fd, &newEvent) == -1)
			throw errno;

		last = nullptr;
	}

	epoll_event eEvent;
	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);

	if (epoll_pwait(efd, &eEvent, 1, -1, &mask) == -1) {
		if (errno == SIGINT) {
			return Event_Data{
				0,
				nullptr
			};
		}
		else {
			throw errno;
		}
	}
	return *(Event_Data*)(eEvent.data.ptr);
#else
	if (last != INVALID_SOCKET) {
		lock.lock();

		for (std::vector<int>::iterator iter = fired.begin(); iter != fired.end(); iter++) {
			if (*iter == last) {
				fired.erase(iter);
				break;
			}
		}

		lock.unlock();
		last = INVALID_SOCKET;
	}

	while (true) {
		if (events.size() == 0)
			SleepEx(INFINITE, true); //To prevent WSA crashes, if no sockets are in the poll, wait indefinitely until awoken
		else {
			int result = WSAWaitForMultipleEvents(events.size(), events.data(), false, WSA_INFINITE, true); //Interruptable select

			if (result == WSA_WAIT_FAILED)
				socketError("Socket poll error: ");
			else if (result != WSA_WAIT_IO_COMPLETION) {
				int offset = result - WSA_WAIT_EVENT_0;

				if (offset < events.size()) {
					INNER * store = (INNER*)tracker[offset];

					if (tryFire(store->data->fd))
						return *(store->data);
				}
				else
					error("Attempted to return from wait, but result was invalid: " + std::to_string(result));
			}
		}

		if (!running)
			return Event_Data{
				0,
				nullptr
			};
	}
#endif
}
