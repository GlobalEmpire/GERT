#pragma once
#include <string>

const struct {
	unsigned char major = 2;
	unsigned char minor = 0;
	unsigned char patch = 0;

	std::string stringify() const { 
		return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch); 
	};

	std::string tostring() const {
		return std::string((char*)&major, 2);
	}
} ThisVersion;
