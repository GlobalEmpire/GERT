#include "../Address.h"
#define ID_LENGTH 3

char* createTunnel(Address, Address);
Address* getTunnel(Address, char*);
void destroyTunnel(Address, char*);
void addTunnel(Address, Address, char*);
