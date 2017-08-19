#ifndef __KEYMNGR__
#define __KEYMNGR__
#include "netDefs.h"
#include "Key.h"

bool checkKey(Address, Key);
void addResolution(Address, Key);
void removeResolution(Address);
#endif
