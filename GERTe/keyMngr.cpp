#include "netDefs.h"
#include <map>

struct GERTkey {
	char key[20] = { 0 };
};

map<GERTaddr, GERTkey> resolutions;

bool checkKey(GERTaddr requested, GERTkey key) {
	return (resolutions[requested].key == key.key);
}

void addResolution(GERTaddr addr, GERTkey key) {
	resolutions[addr] = key;
}

void removeResolution(GERTaddr addr) {
	resolutions.erase(addr);
}