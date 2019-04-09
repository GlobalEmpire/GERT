#include "Connection.h"
#include "Key.h"
#include <map>

std::map<Address, Key> resolutions;

Key::Key(Connection * conn) {
	char * data = conn->read(20);
	key = std::string{ data + 1, 20 };
	delete data;
}

void Key::add(Address addr, Key key) {
	resolutions[addr] = key;
}

void Key::remove(Address addr) {
	resolutions.erase(addr);
}

bool Key::check(Address target) {
	if (resolutions.count(target) > 0 && resolutions[target] == *this)
		return true;
	return false;
}