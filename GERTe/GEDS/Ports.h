#ifndef __NETDEFS__
#define __NETDEFS__
#include <string>
#include <netinet/ip.h>
using namespace std;

class Ports {
	public:
		unsigned short gate, peer;
		Ports(unsigned short gatePort, unsigned short peerPort) : gate(htons(gatePort)), peer(htons(peerPort)) {};
		Ports() : gate(0), peer(0) {};
		string stringify() { return to_string(ntohs(gate)) + ":" + to_string(ntohs(peer)); };
};
#endif
