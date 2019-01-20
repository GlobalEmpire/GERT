#pragma once

#include <string>

const struct {
	unsigned char major, minor, patch;

	std::string stringify() const { 
		return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch); 
	};
} ThisVersion;
