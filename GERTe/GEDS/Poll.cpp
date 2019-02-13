#include "Poll.h"
#include "logging.h"

#ifndef _WIN32
#include <sys/epoll.h>
#include <unistd.h>
#else
#include <WinSock2.h>
#include <thread>
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
#define GETFD (*iter)->fd
typedef std::vector<Event_Data*> TrackerT;
#endif

void removeTracker(SOCKET fd, Poll* context) {
	TrackerT tracker = context->tracker;

	int i = 0;
	for (TrackerT::iterator iter = tracker.begin(); iter != tracker.end(); iter++)
	{
		i++;

#ifdef _WIN32
		INNER * store = (INNER*)(*iter);
#endif

		if (GETFD == fd) {
#ifdef _WIN32
			std::vector<void*>::iterator eiter = context->events.begin() + i;
			WSACloseEvent(*eiter);
			context->events.erase(eiter);

			delete store->data;
#endif
			delete *iter;

			tracker.erase(iter);
			tracker.shrink_to_fit();

			break;
		}
	}
}

Poll::Poll() {
#ifndef _WIN32
	efd = epoll_create(1);
#else
	handler = GetCurrentThread();
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
	WSAEventSelect(fd, e, FD_READ || FD_ACCEPT || FD_CLOSE);

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
#else
	events.erase(events.end()--);
	events.shrink_to_fit();
#endif

	removeTracker(fd, this);
}

Event_Data Poll::wait() { //Awaits for an event on a file descriptor. Returns the Event_Data for a single event
#ifndef _WIN32
	epoll_event eEvent;

	if (epoll_wait(efd, &eEvent, 1, -1) == -1)
		throw errno;
	return *(Event_Data*)(eEvent.data.ptr);
#else
	while (true) {
		int result = WSAWaitForMultipleEvents(events.size(), events.data(), false, WSA_INFINITE, true); //Interruptable select

		if (result != WSA_WAIT_IO_COMPLETION) {
			INNER * store = (INNER*)tracker[result - WSA_WAIT_EVENT_0];

			return *(store->data);
		}
	}
#endif
}
