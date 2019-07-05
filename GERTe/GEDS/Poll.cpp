#include "Poll.h"
#include "logging.h"
#include "Error.h"

#ifndef _WIN32
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#else
#include <WinSock2.h>
#include <mutex>
#endif

extern volatile bool running;

thread_local INet * last = nullptr;

#ifndef _WIN32
sigset_t mask;
#endif

#ifdef _WIN32
inline INet* Poll::WSALoop() {
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
					INet* obj = tracker[offset];

					if (obj->lock.try_lock()) {
						return obj;
					}
				}
				else
					error("Attempted to return from wait, but result was invalid: " + std::to_string(result));
			}
			else
				return nullptr;
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
}

void Poll::add(INet * ptr) { //Adds the file descriptor to the pollset and tracks useful information
#ifndef _WIN32
	epoll_event newEvent;

	newEvent.events = EPOLLIN | EPOLLONESHOT;

	newEvent.data.ptr = ptr;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, ptr->sock, &newEvent) == -1)
		throw errno;

	tracker.push_back(ptr);
#else
	WSAEVENT e = WSACreateEvent();
	WSAEventSelect(ptr->sock, e, FD_READ | FD_ACCEPT | FD_CLOSE);

	tracker.push_back(ptr);
	events.push_back(e);
#endif
}

void Poll::remove(INet* target) {
#ifdef _WIN32
	std::vector<void*>::iterator eiter = events.begin();	
#endif
	for (std::vector<INet*>::iterator iter = tracker.begin(); iter != tracker.end(); iter++)
	{
		INet* obj = *iter;

		if (obj == target) {
#ifdef _WIN32
			WSACloseEvent(*eiter);
			events.erase(eiter);
			events.shrink_to_fit();
#endif
			tracker.erase(iter);
			tracker.shrink_to_fit();

			return;
		}
#ifdef _WIN32
		eiter++;
#endif
	}

	if (last != nullptr && last == target) {
#ifdef _WIN32
		last->lock.unlock();
#endif
		last = nullptr;
	}
#ifndef _WIN32
	if (epoll_ctl(efd, EPOLL_CTL_DEL, target->sock, nullptr) == -1)
		throw errno;
#endif
}

INet* Poll::wait() { //Awaits for an event on a file descriptor. Returns the Event_Data for a single event
	if (last != nullptr) {
#ifdef _WIN32
		last->lock.unlock();
#else
		epoll_event newEvent;
		newEvent.events = EPOLLIN | EPOLLONESHOT;
		newEvent.data.ptr = last;

		if (epoll_ctl(efd, EPOLL_CTL_MOD, last->sock, &newEvent) == -1)
			throw errno;
#endif
		last = nullptr;
	}

	INet* obj;

	start:
#ifndef _WIN32
	epoll_event eEvent;

	if (epoll_pwait(efd, &eEvent, 1, -1, &mask) == -1)
		if (errno == SIGINT)
			obj = nullptr;
		else
			throw errno;

	obj = (INet*)eEvent.data.ptr;
#else
	obj = WSALoop();
#endif
	char test;

	if (obj == nullptr)
		return obj;
	else if (obj->type == INet::Type::CONNECT && recv(obj->sock, &test, 1, MSG_PEEK) != 1) {
		delete obj;
		goto start;
	}
	else {
		last = obj;
		return obj;
	}
}
