#pragma once
#include <string>

constexpr int tunnelVersion = 2;

const struct {
	unsigned char major = 1;
	unsigned char minor = 2;
	unsigned char patch = 0;

	std::string stringify() const { 
		return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch); 
	};

	std::string tostring() const {
		return std::string((char*)&major, 2);
	}
} ThisVersion;
