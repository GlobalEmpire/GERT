#include "Connection.h"
#include "Poll.h"

namespace std {
	struct thread;
}

class Processor {
	Poll * poll;

	unsigned int poolsize;
	std::thread * pool;
public:
	Processor(Poll *);
	~Processor();

	void update();
};
