#ifndef __LIBLOAD__
#define __LIBLOAD__
#include "libDefs.h"
#include "Status.h"
using namespace std;

Status loadLibs();
version* getVersion(UCHAR);
UCHAR highestVersion();
#endif
