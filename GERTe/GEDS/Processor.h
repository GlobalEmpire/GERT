#include "Connection.h"
#include "Poll.h"

class Processor {
	Poll * poll;

	unsigned int poolsize;
	void * pool;

	void run();
public:
	Processor(Poll *);
	~Processor();

	void update();
};
