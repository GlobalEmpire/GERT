#include "../Address.h"

struct Tunnel {
	Address start;
	Address end;
};

char* createTunnel(Address, Address);
Tunnel* getTunnel(char*);
void destroyTunnel(char*);
