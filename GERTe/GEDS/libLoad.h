#ifndef __LIBLOAD__
#define __LIBLOAD__
#include "libDefs.h"
using namespace std;

enum libErrors {
	NO_ERR,
	UNKNOWN,
	EMPTY
};

int loadLibs();
version* getVersion(UCHAR);
UCHAR highestVersion();
#endif
