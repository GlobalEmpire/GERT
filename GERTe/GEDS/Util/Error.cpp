#include "Error.h"
#include "logging.h"

#ifdef _WIN32
#define GET_GENERAL_ERROR GetLastError()
#define GET_SOCKET_ERROR WSAGetLastError()

#include <WinSock2.h>
#else
#define GET_GENERAL_ERROR errno
#define GET_SOCKET_ERROR errno

#include <errno.h>
#include <system_error>

#endif

thread_local bool recursionCheck = false;

std::string printError(int, std::string); //Fixes circular dependency

std::string socketError(const std::string& prefix) {
    return printError(GET_SOCKET_ERROR, prefix);
}

std::string generalError(const std::string& prefix) {
    return printError(GET_GENERAL_ERROR, prefix);
}

#ifdef _WIN32
std::string formaterror() {
	if (recursionCheck) {
		error("Whoops! Looping in error handling function. Passing the code to terminate.");
		errno = (int)GetLastError();
		terminate();
	}

	recursionCheck = true;
	auto err = printError((int)GetLastError(), "Error when parsing a previous error: ");
	recursionCheck = false;

	return err;
}
#endif

std::string printError(int code, std::string msg) { //Inlined to minimize overhead
    if (!msg.ends_with(' '))
        msg += ' ';

#ifdef _WIN32
	LPSTR err = nullptr;

	if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      nullptr, code, 0, (LPSTR)&err, 0, nullptr) == 0)
		return formaterror();
	else
        msg += err;

    if (LocalFree(err) != nullptr)
        return formaterror();
    else
        return msg;
#else
	return msg + std::make_error_code((std::errc)code).message();
#endif
}
