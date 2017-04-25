#ifndef __KEYMNGR__
#define __KEYMNGR__
#include "netDefs.h"

class GERTkey {
	public:
		char key[20] = {0};
		bool operator = (const GERTkey comp) const { return (key == comp.key); };
};

bool checkKey(GERTaddr, GERTkey);
void addResolution(GERTaddr, GERTkey);
void removeResolution(GERTaddr);
#endif
