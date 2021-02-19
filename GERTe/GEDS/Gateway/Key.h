#pragma once
#include "Address.h"
#include <map>

class Key {
	public:
        std::string key;
        inline static std::map<Address, Key> resolutions;

		bool operator== (const Key& comp) const;
		explicit Key(std::string);

		static bool exists(Address);
		static Key retrieve(Address);
};