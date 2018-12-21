#pragma once

#include "Connection.h"
#include <string>

class Key {
	friend void saveResolutions();

	std::string key{20, 0};
	public:
		bool operator== (const Key& comp) const { return (key == comp.key); };
		Key (const std::string& keyin) : key(keyin) {
			key.resize(20);
		};
		Key () {};
		Key static extract(Connection*);
};