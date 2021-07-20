#pragma once
#include "../Networking/netty.h"
#include <vector>

#ifdef WIN32
#include <mutex>
#include <map>

struct INNER;
#endif

class Poll {
	void removeTracker(Socket*);

#ifndef _WIN32
	int efd;
	std::vector<Socket*> tracker;
#else
	std::mutex lock;
	std::map<Socket*, INNER*> fired;

	std::vector<INNER*> tracker;
	std::vector<void*> events;
#endif

public:
#ifndef WIN32
	Poll();
#endif

	~Poll();

	void add(Socket*);
	void remove(Socket*);

	Socket* wait();

    void clear(Socket*);
};
