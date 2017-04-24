#include <string>
using namespace std;

enum libErrors {
	NORMAL,
	UNKNOWN,
	EMPTY
};

struct versioning {
	UCHAR major, minor, patch;
};

class version {
	public:
		bool(*procGate)(gateway, string);
		bool(*procPeer)(peer, string);
		void(*killGate)(gateway);
		void(*killPeer)(peer);
		versioning vers;
		void* handle;
};

int loadLibs();
version* getVersion(UCHAR);
