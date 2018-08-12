#include "Poll.h"
#include <sys/epoll.h>
#include <vector>

typedef vector<Event_Data*> LList;

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
	efd = epoll_create(0);

	tracker = new LList;
}

Poll::~Poll() {
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
}

void Poll::add(SOCKET fd, void * ptr = nullptr) { //Adds the file descriptor to the pollset and tracks useful information
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
}

void Poll::remove(SOCKET fd) {
	if (epoll_ctl(efd, EPOLL_CTL_DEL, fd, nullptr) == -1)
		throw errno;

	removeTracker(fd, (LList*)tracker);
}

Event_Data Poll::wait() { //Awaits for an event on a file descriptor. Returns the Event_Data for a single event
	epoll_event eEvent;

	if (epoll_wait(efd, &eEvent, 1, -1) == -1)
		throw errno;

	return *(eEvent.data.ptr);
}
