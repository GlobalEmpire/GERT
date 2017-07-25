#include "keyMngr.h"
#include <map>
using namespace std;

map<GERTaddr, GERTkey> resolutions; //Create key database

bool checkKey(GERTaddr requested, GERTkey key) { //Check if the key matches the address
	return (resolutions[requested] == key);
}

void addResolution(GERTaddr addr, GERTkey key) { //Add key for the address
	resolutions[addr] = key;
}

void removeResolution(GERTaddr addr) { //Remove the key for the address
	resolutions.erase(addr);
}
