#ifndef __LIBLOAD__
#define __LIBLOAD__
#include <string>
using namespace std;

typedef unsigned char UCHAR;

class gateway;
class peer;

enum libErrors {
	NO_ERR,
	UNKNOWN,
	EMPTY
};

struct versioning {
	UCHAR major, minor, patch;
};

class version {
	public:
		bool(*procGate)(gateway*, string);
		bool(*procPeer)(peer*, string);
		void(*killGate)(gateway*);
		void(*killPeer)(peer*);
		versioning vers;
		void* handle;
};

int loadLibs();
version* getVersion(UCHAR);
#endif
