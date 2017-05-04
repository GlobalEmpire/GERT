#ifndef __KEYMNGR__
#define __KEYMNGR__
#include "netDefs.h"
#include "keyDef.h"

bool checkKey(GERTaddr, GERTkey);
void addResolution(GERTaddr, GERTkey);
void removeResolution(GERTaddr);
#endif
