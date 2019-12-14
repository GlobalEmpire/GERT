#include "Error.h"
#include "logging.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>

#define GET_SOCK_ERROR WSAGetLastError()
#define GET_GEN_ERROR GetLastError()
#else
#include <errno.h>

#define GET_SOCK_ERROR errno
#define GET_GEN_ERROR errno
#endif

#ifdef _WIN32
[[noreturn]]
void formaterror() {
	error("Whoops! Error in error handling. Passing code to terminate");
	errno = GetLastError();
	terminate();
}
#endif

std::string inline printError(int code, std::string msg) { //Inlined to minimize overhead
#ifdef _WIN32
	LPSTR err = NULL;
	int result = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0, (LPSTR)&err, 0, NULL);

	if (result == 0)
		formaterror();

	msg = msg + err;

	HLOCAL freeres = LocalFree(err);

	if (freeres != NULL)
		formaterror();

	return msg;
#else
	return msg + std::to_string(code) + "\n";
#endif
}

std::string socketError(std::string prefix) {
	return printError(GET_SOCK_ERROR, prefix);
}

std::string generalError(std::string prefix) {
	return printError(GET_GEN_ERROR, prefix);
}

std::string knownError(int code, std::string prefix) {
	return printError(code, prefix);
}

[[noreturn]]
void crash(ErrorCode code) {
	exit((int)code);
}
