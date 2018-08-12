#include "Key.h"
#include "Address.h"
#include <map>

std::map<Address, Key> resolutions;

Key Key::extract(Connection * conn) {
	char * data = conn->read(20);
	Key key = std::string{data + 1, 20};
	delete data;

	return key;
}

extern "C" {
	void addResolution(Address addr, Key key) {
		resolutions[addr] = key;
	}

	void removeResolution(Address addr) {
		resolutions.erase(addr);
	}
}
