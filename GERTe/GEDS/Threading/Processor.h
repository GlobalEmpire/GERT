#include "../Networking/Connection.h"
#include "Poll.h"

class Processor {
	Poll * poll;
	void * proc;

	void run();
public:
	Processor(Poll *);
	~Processor();
};
