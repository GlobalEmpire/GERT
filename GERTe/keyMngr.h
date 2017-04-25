#ifndef __KEYMNGR__
#define __KEYMNGR__
#include "netDefs.h"

class GERTkey {
	public:
		string key{20, 0};
		bool operator = (const GERTkey comp) const { return (key == comp.key); };
		GERTkey (string keyin) : key(keyin) {
			key.resize(20);
		};
};

bool checkKey(GERTaddr, GERTkey);
void addResolution(GERTaddr, GERTkey);
void removeResolution(GERTaddr);
#endif
