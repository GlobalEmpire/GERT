#include "Connection.h"
#include "Key.h"
#include <map>

std::map<Address, Key> resolutions;

Key::Key(const char* keyin) {
	key.assign(keyin, 20);
}

Key::Key(Connection * conn, int offset) {
	key = std::string{ conn->buf + offset, 20 };
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