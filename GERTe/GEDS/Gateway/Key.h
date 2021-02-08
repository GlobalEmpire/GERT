#pragma once
#include "Address.h"

class Key {
	friend void saveResolutions();

	public:
        void* key;

		bool operator== (const Key& comp) const;
		explicit Key (const std::string&);

		static void add(Address, Key);
		static void remove(Address);

		static bool exists(Address);
		static Key retrieve(Address);
};