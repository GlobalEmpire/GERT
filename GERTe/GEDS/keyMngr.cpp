#include "netDefs.h"
#include <map>
using namespace std;

class GERTkey {
	public:
		char key[20] = {0};
		bool operator= (const GERTkey comp) const { return (key == comp.key); };
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
