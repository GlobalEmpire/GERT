#define ID_LENGTH 3

#include "Tunnel.h"
#include <random>
#include <map>
using namespace std;

random_device device;
default_random_engine gen(device());
uniform_int_distribution<short> dis(0, 255);

class String {
	char * ptr;

public:
	String(char * start) : ptr(start) {};
	bool operator<(const String& rhs) const {
		for (int i = 0; i < ID_LENGTH; i++) {
			if (this->ptr[i] < rhs.ptr[i])
				return true;
			else if (this->ptr[i] > rhs.ptr[i])
				return false;
		}
		return false;
	}
};

map<String, Tunnel> tunnels;

char * genID() {
	char * id = new char[ID_LENGTH];
	for (int i = 0; i < ID_LENGTH; i++) {
		id[i] = (char)dis(gen);
	}

	return id;
}

char* createTunnel(Address start, Address end) {
	char * id = genID();

	while (tunnels.count(String{ID}) == 1) {
		delete id;
		id = genID();
	}

	return Tunnel{
		id,
		start,
		end
	};
}

void destroyTunnel(char * id) {
	tunnels.erase(String{id});
}

Tunnel* getTunnel(char * id) {
	if (tunnels.count(String{id}) == 0)
		return nullptr;
	return &tunnels[String{id}];
}
