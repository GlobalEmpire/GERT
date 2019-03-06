#include "Poll.h"
#include "logging.h"
#include <thread>

#ifndef _WIN32
#include <sys/epoll.h>
#include <unistd.h>
#include <signal.h>
#else
#include <WinSock2.h>
#endif

extern volatile bool running;

#ifdef _WIN32
#define GETFD store->data->fd
struct INNER {
	WSAEVENT * event;
	Event_Data * data;
};

typedef std::vector<void*> TrackerT;

void apc(ULONG_PTR s) {}
#else
#define GETFD store->fd
typedef std::vector<Event_Data*> TrackerT;
#endif

void removeTracker(SOCKET fd, Poll* context) {
	TrackerT tracker = context->tracker;

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

	delete (pthread_t*)handler;
#else
	for (void * data : tracker) {
		INNER * store = (INNER*)data;

		delete store->data;
		WSACloseEvent(store->event);

		delete store;
	}

	CloseHandle(handler);
#endif
}

void Poll::add(SOCKET fd, void * ptr) { //Adds the file descriptor to the pollset and tracks useful information
	Event_Data * data = new Event_Data{
		fd,
		ptr
	};

#ifndef _WIN32
	epoll_event newEvent;

	newEvent.events = EPOLLIN;

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

	QueueUserAPC(apc, handler, 0); //Cause blocked thread to return allowing it to update it's select call
#endif
}

void Poll::remove(SOCKET fd) {
#ifndef _WIN32
	if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr) == -1)
		throw errno;
#endif

	removeTracker(fd, this);
}

Event_Data Poll::wait() { //Awaits for an event on a file descriptor. Returns the Event_Data for a single event
#ifndef _WIN32
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
	while (true) {
		if (events.size() == 0)
			SleepEx(INFINITE, true); //To prevent WSA crashes, if no sockets are in the poll, wait indefinitely until awoken
		else {
			int result = WSAWaitForMultipleEvents(events.size(), events.data(), false, WSA_INFINITE, true); //Interruptable select

			if (result == WSA_WAIT_FAILED) {
				error("Unknown WSA failure. Code: " + std::to_string(WSAGetLastError()));
			}
			else if (result != WSA_WAIT_IO_COMPLETION) {
				int offset = result - WSA_WAIT_EVENT_0;

				if (offset < events.size()) {
					INNER * store = (INNER*)tracker[offset];

					return *(store->data);
				}
				else {
					error("Attempted to return from wait, but result was invalid: " + std::to_string(result));
				}
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

void Poll::claim() {
#ifdef _WIN32
	if (handler != nullptr) {
		error("Attempt to double claim poll!");
		exit(3);
	}

	DWORD id = GetCurrentThreadId();
	handler = OpenThread(THREAD_ALL_ACCESS, false, id);
#else
	handler = new pthread_t;
	*(pthread_t*)handler = pthread_self();
#endif
}

void Poll::update() {
#ifdef _WIN32
	QueueUserAPC(apc, handler, 0);
#else
	pthread_kill(*(std::thread::native_handle_type*)handler, SIGUSR1);
#endif
}
