#ifndef __KEY_GUARD__
#define __KEY_GUARD__

#include <string>
#include "Connection.h"
using namespace std;

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

#endif
