#include "Error.h"
#include "logging.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#else
#include <errno.h>
#endif

thread_local bool recursionCheck = false;

void inline printError(int, std::string); //Fixes circular dependency

#ifdef _WIN32
void inline formaterror() {
	if (recursionCheck) {
		error("Whoops! Looping in error handling function. Passing the code to terminate.");
		errno = GetLastError();
		terminate();
	}

	recursionCheck = true;
	printError(GetLastError(), "Error when parsing a previous error: ");
	recursionCheck = false;
}

void inline cleanup(HLOCAL buffer) {
	HLOCAL result = LocalFree(buffer);
	
	if (result != NULL)
		formaterror();
}
#endif

void socketError(std::string prefix) {
#ifdef _WIN32
	printError(WSAGetLastError(), prefix);
#else
	printError(errno, prefix);
#endif
}

void generalError(std::string prefix) {
#ifdef _WIN32
	printError(GetLastError(), prefix);
#else
	printError(errno, prefix);
#endif
}

void knownError(int code, std::string prefix) {
	printError(code, prefix);
}

void inline printError(int code, std::string prefix) { //Inlined to minimize overhead
#ifdef _WIN32
	LPSTR err = NULL;
	int result = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0, (LPSTR)&err, 0, NULL);

	if (result == 0)
		formaterror();
	else
		error2(prefix + err);

	cleanup(err);
#else
	error(prefix + std::to_string(code));
#endif
}
