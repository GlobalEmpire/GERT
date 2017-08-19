#ifndef __LIBDEFS__
#define __LIBDEFS__
#include <string>
using namespace std;

typedef unsigned char UCHAR;

class Gateway;
class peer;

class versioning {
	public:
		UCHAR major, minor, patch;
		string stringify() { return to_string(major) + "." + to_string(minor) + "." + to_string(patch); };
};


class version {
	public:
		bool(*procGate)(Gateway*, string);
		bool(*procPeer)(peer*, string);
		void(*killGate)(Gateway*);
		void(*killPeer)(peer*);
		versioning vers;
		void* handle;
};
#endif
