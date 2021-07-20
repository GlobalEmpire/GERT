#pragma once
#include "Address.h"

class GERTc {
public:
	Address external;
	Address internal;
	GERTc(const std::string& data) : external(data.substr(0, 3)), internal(data.substr(3, 3)) {};
	GERTc() : external(), internal() {};
	std::string stringify() const { return external.stringify() + ":" + internal.stringify(); }
	std::string tostring() const { return external.tostring() + internal.tostring(); }
};