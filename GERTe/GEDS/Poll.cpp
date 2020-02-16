#include "Poll.h"
#include "logging.h"
#include "Error.h"

#ifndef _WIN32
#define INVALID_SOCKET ~0

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#else
#include <WinSock2.h>
#include <mutex>
#endif

extern volatile bool running;

thread_local SOCKET this_sock = INVALID_SOCKET;

#ifndef _WIN32
bool init = false;
sigset_t mask;
#endif

#ifdef _WIN32
inline INet* Poll::WSALoop() {
	while (true) {
		if (events.size() == 0)
			SleepEx(INFINITE, true); //To prevent WSA crashes, if no sockets are in the poll, wait indefinitely until awoken
		else {
			int result = WSAWaitForMultipleEvents((DWORD)events.size(), events.data(), false, WSA_INFINITE, true); //Interruptable select

			if (result == WSA_WAIT_FAILED) {
				int err = WSAGetLastError();
				if (err != WSA_INVALID_HANDLE)
					error(knownError(err, "Socket poll error: "), false);
			}
			else if (result != WSA_WAIT_IO_COMPLETION) {
				result -= WSA_WAIT_EVENT_0;

				if (result < events.size()) {
					INet* obj = tracker[result];

					WSAResetEvent(events[result]);

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
#else
inline INet* Poll::linuxLoop() {
	epoll_event eEvent;

	if (epoll_pwait(efd, &eEvent, 1, -1, &mask) == -1)
		if (errno == EINTR)
			return nullptr;
		else
			throw errno;

	return (INet*)eEvent.data.ptr;
}
#endif

Poll::Poll() {
#ifndef _WIN32
	efd = epoll_create(1);

	if (!init) {
		sigemptyset(&mask);
		sigaddset(&mask, SIGUSR1);

		init = true;
	}
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
#else
	WSAEVENT e = WSACreateEvent();
	WSAEventSelect(ptr->sock, e, FD_READ | FD_ACCEPT | FD_CLOSE);

	events.push_back(e);
#endif

	tracker.push_back(ptr);
}

void Poll::remove(SOCKET target) {
#ifdef _WIN32
	std::vector<void*>::iterator eiter = events.begin();	
#endif
	for (std::vector<INet*>::iterator iter = tracker.begin(); iter != tracker.end(); iter++)
	{
		INet* obj = *iter;

		if (obj->sock == target) {
#ifdef _WIN32
			WSACloseEvent(*eiter);
			events.erase(eiter);
			events.shrink_to_fit();
#endif
			tracker.erase(iter);
			tracker.shrink_to_fit();

			break;
		}
#ifdef _WIN32
		eiter++;
#endif
	}

	if (this_sock != INVALID_SOCKET && this_sock == target) {
		this_sock = INVALID_SOCKET;
	}
#ifndef _WIN32
	if (epoll_ctl(efd, EPOLL_CTL_DEL, target, nullptr) == -1 && errno != EBADF)
		throw errno;
#endif
}

void Poll::wait() { //Awaits for an event on a file descriptor. Returns the Event_Data for a single event
	while (running) {
#ifndef _WIN32
		INet* obj = linuxLoop();
#else
		INet * obj = WSALoop();
#endif
		if (obj == nullptr)
			continue;
		else if (obj->type == INet::Type::CONNECT && ((IConsumer*)obj)->isClosed()) {
			delete obj;
		else {
		}
		else if ((obj->type == INet::Type::CONNECT && ((IConsumer*)obj)->querySocket()) || obj->type == INet::Type::LISTEN) {
			this_sock = obj->sock;

		process:
			obj->process();

			if (this_sock != INVALID_SOCKET) {
				if (obj->type != INet::Type::CONNECT && ((IConsumer*)obj)->querySocket())
					goto process;
#ifndef _WIN32
				epoll_event newEvent;
				newEvent.events = EPOLLIN | EPOLLONESHOT;
				newEvent.data.ptr = obj;

				if (epoll_ctl(efd, EPOLL_CTL_MOD, obj->sock, &newEvent) == -1)
					throw errno;
#endif
				obj->lock.unlock();
			}

			this_sock = INVALID_SOCKET;
		}
	}
}
