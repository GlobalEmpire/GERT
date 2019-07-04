#pragma once

#include "Address.h"

class Key {
	friend void saveResolutions();

	std::string key{20, 0};
	public:
		bool operator== (const Key& comp) const { return (key == comp.key); };
		Key(const char*);
		Key(Connection*, int);
		Key () {};

		bool check(Address);
		static void add(Address, Key);
		static void remove(Address);
};