#include "Trace.h"
#include "logging.h"

#ifndef _WIN32
#include <execinfo.h>
#endif

void dumpStack() {
#ifdef _WIN32
	error("Windows stack dump not implemented!");
#else
	void * buffer[256];
	FILE * dump = fopen("error.dump", "w");
	if (dump == nullptr) {
		error("Failed to dump stack");
		return;
	}
	int filenum = fileno(dump);
	int num = backtrace(buffer, 256);
	backtrace_symbols_fd(buffer, num, filenum);
#endif
}
