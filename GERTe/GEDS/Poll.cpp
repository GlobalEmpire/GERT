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
thread_local INet* last = nullptr;
#else
thread_local Event_Data * last = nullptr;
#endif

typedef std::vector<Event_Data*> TrackerT;

void removeTracker(SOCKET fd, Poll* context) {
	TrackerT &tracker = context->tracker;

	int i = 0;
	for (TrackerT::iterator iter = tracker.begin(); iter != tracker.end(); iter++)
	{
		Event_Data * store = *iter;

		if (store->fd == fd) {
#ifdef _WIN32
			std::vector<void*>::iterator eiter = context->events.begin() + i;

			WSACloseEvent(*eiter);
			context->events.erase(eiter);
			context->events.shrink_to_fit();
#endif
			delete store;

			tracker.erase(iter);
			tracker.shrink_to_fit();

			return;
		}

		i++;
	}
}

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
#endif

	for (Event_Data* data : tracker)
	{
		delete data;
	}
}

void Poll::add(SOCKET fd, INet * ptr) { //Adds the file descriptor to the pollset and tracks useful information
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

	tracker.push_back(data);
	events.push_back(e);
#endif
}

void Poll::remove(SOCKET fd) {
#ifndef _WIN32
	if (last != nullptr && last->fd == fd)
		last = nullptr;
#endif

	removeTracker(fd, this);

#ifdef _WIN32
	if (last != nullptr && last->sock == fd) {
		last->lock.unlock();
		last = nullptr;
	}
#endif
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
	if (last != nullptr) {
		last->lock.unlock();
	}

	while (true) {
		if (events.size() == 0)
			SleepEx(INFINITE, true); //To prevent WSA crashes, if no sockets are in the poll, wait indefinitely until awoken
		else {
			int result = WSAWaitForMultipleEvents(events.size(), events.data(), false, WSA_INFINITE, true); //Interruptable select

			if (result == WSA_WAIT_FAILED) {
				int err = WSAGetLastError();
				if (err != WSA_INVALID_HANDLE)
					knownError(err, "Socket poll error: ");
			}
			else if (result != WSA_WAIT_IO_COMPLETION) {
				int offset = result - WSA_WAIT_EVENT_0;

				if (offset < events.size()) {
					Event_Data * store = tracker[offset];
					INet* obj = store->ptr;

					if (obj->lock.try_lock()) {
						last = obj;
						return *(store);
					}
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
