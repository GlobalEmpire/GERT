#pragma once

#include "Connection.h"
#include "Address.h"

class Key {
	friend void saveResolutions();

	std::string key{20, 0};
	public:
		bool operator== (const Key& comp) const { return (key == comp.key); };
		Key (const std::string& keyin) : key(keyin) {
			key.resize(20);
		};
		Key () {};
		bool check(Address);

		static Key extract(Connection*);
		static void add(Address, Key);
		static void remove(Address);
};