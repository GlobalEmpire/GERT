#include "../Address.h"

struct Tunnel {
	char * id;
	Address start;
	Address end;
};

char* createTunnel(Address, Address);
Tunnel* getTunnel(char*);
void destroyTunnel(char*);
