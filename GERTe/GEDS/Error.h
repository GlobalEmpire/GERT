#include <string>

enum class ErrorCode {
	PEER_LOAD_ERROR = 1,
	KEY_LOAD_ERROR,
	LIBRARY_ERROR,
	GENERAL_ERROR
};

void socketError(std::string);
void generalError(std::string);
void knownError(int, std::string);

void crash(ErrorCode);
