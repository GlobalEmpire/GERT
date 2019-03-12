#include "Connection.h"
#include "Poll.h"

class Processor {
	friend void worker(void*);

	Poll * poll;
	void * pool;

	void run();
public:
	Processor(Poll *);
	~Processor();

	void update();
};
