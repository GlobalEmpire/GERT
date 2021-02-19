#include "../Networking/Connection.h"
#include "Poll.h"

class Processor {
	Poll * poll;

	unsigned int numThreads;
	void* threads;

	void run();
public:
	explicit Processor(Poll *);
	~Processor();
};
