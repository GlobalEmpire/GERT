#include <string>

enum class ErrorCode {
	PEER_LOAD_ERROR = 1,
	KEY_LOAD_ERROR,
	LIBRARY_ERROR,
	GENERAL_ERROR
};

std::string socketError(std::string);
std::string generalError(std::string);
std::string knownError(int, std::string);

void crash(ErrorCode);
