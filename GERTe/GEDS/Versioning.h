#include <string>
using namespace std;

class Versioning {
public:
	unsigned char major, minor, patch;
	string stringify() { return to_string(major) + "." + to_string(minor) + "." + to_string(patch); }
};
