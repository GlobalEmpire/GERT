#include "Trace.h"
#include "logging.h"
#include <execinfo.h>

void dumpStack() {
	void * buffer[256];
	FILE * dump = fopen("error.dump", "w");
	if (dump == nullptr) {
		error("Failed to dump stack");
		return;
	}
	int num = backtrace(buffer, 256);
	int filenum = fileno(dump);
	backtrace_symbols_fd(buffer, num, filenum);
}
