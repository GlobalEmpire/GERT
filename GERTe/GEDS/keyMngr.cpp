#include "keyMngr.h"
#include <map>

map<Address, Key> resolutions; //Create key database

bool checkKey(Address requested, Key key) { //Check if the key matches the address
	return (resolutions[requested] == key);
}

void addResolution(Address addr, Key key) { //Add key for the address
	resolutions[addr] = key;
}

void removeResolution(Address addr) { //Remove the key for the address
	resolutions.erase(addr);
}
