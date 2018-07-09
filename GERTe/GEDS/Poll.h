#pragma once

struct Event_Data {
	int fd;
	void * ptr;
};

class Poll {
	int efd;
	void * tracker;

public:
	Poll();
	~Poll();

	void add(int, void*);
	void remove(int);
	Event_Data wait();
};
