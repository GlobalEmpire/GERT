#include "Poll.h"

#ifndef _WIN32
#include <sys/epoll.h>
#include <unistd.h>
#else
#include <WinSock2.h>
typedef std::map<SOCKET, Event_Data*> SList;
#endif

#ifndef _WIN32
typedef std::vector<Event_Data*> LList;

void removeTracker(int fd, LList* tracker) {
	for (LList::iterator iter = tracker->begin(); iter != tracker->end(); iter++)
	{
		if ((*iter)->fd == fd) {
			delete *iter;

			tracker->erase(iter);
		}
	}
}
#endif

Poll::Poll() {
#ifndef _WIN32
	efd = epoll_create(0);
#endif
}

Poll::~Poll() {
#ifndef _WIN32
	close(efd);

	for (Event_Data * data : tracker)
	{
		if (data->ptr == nullptr)
			close(data->fd);
		else
			delete data->ptr;

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
	viewing.lock();
	socks[fd] = data;
	viewing.unlock();
#endif
}

void Poll::remove(SOCKET fd) {
#ifndef _WIN32
	if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr) == -1)
		throw errno;

	removeTracker(fd, &tracker);
#else
	viewing.lock();
	delete socks[fd];
	socks.erase(fd);
	viewing.unlock();
#endif
}

Event_Data Poll::wait() { //Awaits for an event on a file descriptor. Returns the Event_Data for a single event
#ifndef _WIN32
	epoll_event eEvent;

	if (epoll_wait(efd, &eEvent, 1, -1) == -1)
		throw errno;
	return *(Event_Data*)(eEvent.data.ptr);
#else
	while (true) {
		viewing.lock();
		SList::iterator iter = socks.begin();
		const SList::iterator end = socks.end();
		char test;
		while (iter != end) {
			if (recv(iter->first, &test, 1, MSG_PEEK) != SOCKET_ERROR) {
				Event_Data data = *iter->second;
				viewing.unlock();
				return data;
			}
		}
		viewing.unlock();
		std::this_thread::yield();
	}
#endif
}
