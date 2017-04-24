#include "netDefs.h"

struct GERTkey {
	char key[20];
};

bool checkKey(GERTaddr, GERTkey);
void addResolution(GERTaddr, GERTkey);
void removeResolution(GERTaddr);