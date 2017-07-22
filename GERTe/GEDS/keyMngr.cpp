#include "keyMngr.h"
#include <map>
using namespace std;

map<GERTaddr, GERTkey> resolutions;

bool checkKey(GERTaddr requested, GERTkey key) {
	return (resolutions[requested] == key);
}

void addResolution(GERTaddr addr, GERTkey key) {
	resolutions[addr] = key;
}

void removeResolution(GERTaddr addr) {
	resolutions.erase(addr);
}
