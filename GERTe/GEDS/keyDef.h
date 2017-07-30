#ifndef __KEYDEF__
#define __KEYDEF__
#include <string>
using namespace std;

class GERTkey {
	public:
		string key{20, 0};
		bool operator== (const GERTkey comp) const { return (key == comp.key); };
		GERTkey (string keyin) : key(keyin) {
			key.resize(20);
		};
		GERTkey () {};
};

#endif
