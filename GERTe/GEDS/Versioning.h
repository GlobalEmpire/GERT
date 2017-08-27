#include <string>

class Versioning {
public:
	unsigned char major, minor, patch;
	std::string stringify() { return to_string(major) + "." + to_string(minor) + "." + to_string(patch); }
};
