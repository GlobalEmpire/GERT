#pragma once
#include <string>

enum class StatusCodes {
	OK,
	GENERAL_ERROR,
	NO_LIBRARY
};

class Status {
public:
	StatusCodes code;
	char * msg;
	Status(StatusCodes, std::string);
	Status(StatusCodes);
};
