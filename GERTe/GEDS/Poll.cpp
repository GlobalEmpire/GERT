#include "Poll.h"
#include <vector>

#ifndef _WIN32
#include <sys/epoll.h>
#endif

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

Poll::Poll() {
#ifndef _WIN32
	efd = epoll_create(0);

	tracker = new LList;
#endif
}

Poll::~Poll() {
#ifndef _WIN32
	close(fd);

	for (Event_Data * data : *(LList*)tracker)
	{
		if (data.ptr == nullptr)
			close(data->fd);
		else
			delete data->ptr;

		delete data;
	}

	delete tracker;
#endif
}

void Poll::add(SOCKET fd, void * ptr) { //Adds the file descriptor to the pollset and tracks useful information
#ifndef _WIN32
	epoll_event newEvent;

	newEvent.events = EPOLLIN;

	Event_Data * data = new Event_Data{
		fd,
		ptr
	};

	newEvent.data.ptr = data;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &newEvent) == -1)
		throw errno;

	LList.push_back(data);
#endif
}

void Poll::remove(SOCKET fd) {
#ifndef _WIN32
	if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr) == -1)
		throw errno;

	removeTracker(fd, (LList*)tracker);
#endif
}

Event_Data Poll::wait() { //Awaits for an event on a file descriptor. Returns the Event_Data for a single event
#ifndef _WIN32
	epoll_event eEvent;

	if (epoll_wait(efd, &eEvent, 1, -1) == -1)
		throw errno;

	return *(eEvent.data.ptr);
#endif
}
