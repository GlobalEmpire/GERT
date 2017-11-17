#include "Tunnel.h"
#include <random>
#include <map>
using namespace std;

random_device device;
default_random_engine gen(device());
uniform_int_distribution<short> dis(0, 255);

class String {
public:
	char * ptr;
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
	bool operator==(const String& comp) {
		for (int i = 0; i < ID_LENGTH; i++) {
			if (this->ptr[i] != comp.ptr[i])
				return false;
		}
		return true;
	}
};

typedef map<String, Address> Tunnel;

map<Address, Tunnel*> tunnels;

String genID() {
	char* id = new char[ID_LENGTH];
	for (int i = 0; i < ID_LENGTH; i++) {
		id[i] = (char)dis(gen);
	}

	return String{id};
}

char* createTunnel(Address start, Address end) {
	String id = genID();
	Tunnel * startTable = tunnels[start];
	Tunnel * endTable = tunnels[start];

	while (startTable->count(id) || endTable->count(id)) {
		delete id.ptr;
		id = genID();
	}

	(*startTable)[id] = end;
	(*endTable)[id] = start;

	return id.ptr;
}

void destroyTunnel(Address ref, char* id) {
	Tunnel * refTable = tunnels[ref];
	String str = String{id};
	if (refTable->count(str) == 0)
		return;

	Tunnel * other = tunnels[(*refTable)[str]];
	other->erase(str);
	refTable->erase(str);
}

Address * getTunnel(Address ref, char* id) {
	if (not tunnels.count(ref))
		return nullptr;
	Tunnel * refTable = tunnels[ref];
	String str = String{id};
	if (not refTable->count(str))
		return nullptr;
	return &(*refTable)[str];
}
