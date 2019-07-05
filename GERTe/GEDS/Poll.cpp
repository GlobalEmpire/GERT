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

thread_local Event_Data * last = nullptr;

#ifndef _WIN32
sigset_t mask;
#endif

bool testForClose(INet* obj) {
	char test;
	return obj->type == INet::Type::CONNECT || recv(obj->sock, &test, 1, MSG_PEEK) != 1;
}

#ifdef _WIN32
inline Event_Data* Poll::WSALoop() {
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
					Event_Data* store = tracker[offset];
					INet* obj = store->ptr;

					if (obj->lock.try_lock()) {
						return store;
					}
				}
				else
					error("Attempted to return from wait, but result was invalid: " + std::to_string(result));
			}
			else
				throw 1;
		}
	}
}
#endif

Poll::Poll() {
#ifndef _WIN32
	efd = epoll_create(1);

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
#endif
}

Poll::~Poll() {
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
#ifdef _WIN32
	std::vector<void*>::iterator eiter = events.begin();	
#endif
	for (std::vector<Event_Data*>::iterator iter = tracker.begin(); iter != tracker.end(); iter++)
	{
		Event_Data* store = *iter;

		if (store->fd == fd) {
#ifdef _WIN32
			WSACloseEvent(*eiter);
			events.erase(eiter);
			events.shrink_to_fit();
#endif
			delete store;

			tracker.erase(iter);
			tracker.shrink_to_fit();

			return;
		}
#ifdef _WIN32
		eiter++;
#endif
	}

	if (last != nullptr && last->fd == fd) {
#ifdef _WIN32
		last->ptr->lock.unlock();
#endif
		last = nullptr;
	}
#ifndef _WIN32
	if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr) == -1)
		throw errno;
#endif
}

Event_Data Poll::wait() { //Awaits for an event on a file descriptor. Returns the Event_Data for a single event
	if (last != nullptr) {
#ifdef _WIN32
		last->ptr->lock.unlock();
#else
		epoll_event newEvent;
		newEvent.events = EPOLLIN | EPOLLONESHOT;
		newEvent.data.ptr = last;

		if (epoll_ctl(efd, EPOLL_CTL_MOD, last->fd, &newEvent) == -1)
			throw errno;
#endif
		last = nullptr;
	}

	Event_Data* store;

	start:
#ifndef _WIN32
	epoll_event eEvent;

	if (epoll_pwait(efd, &eEvent, 1, -1, &mask) == -1)
		if (errno == SIGINT)
			return Event_Data{
				0,
				nullptr
			};
		else
			throw errno;

	store = eEvent.data.ptr;
#else
	try {
		store = WSALoop();
	}
	catch (int e) {
		return Event_Data{
			0,
			nullptr
		};
	}
#endif
	if (testForClose(store->ptr)) {
		delete store->ptr;
		goto start;
	}
	else {
		last = store;
		return *store;
	}
}
