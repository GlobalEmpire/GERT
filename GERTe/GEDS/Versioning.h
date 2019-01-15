#pragma once

#include <string>

class Versioning {
public:
	unsigned char major, minor, patch;

	std::string stringify() const { 
		return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch); 
	};
};
