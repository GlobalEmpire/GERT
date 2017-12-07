#ifndef __NETDEFS__
#define __NETDEFS__
#include <string>
#include <netinet/ip.h>
#include "Connection.h"
using namespace std;

class Ports {
	public:
		unsigned short gate, peer;
		Ports(unsigned short gatePort, unsigned short peerPort) : gate(gatePort), peer(peerPort) {};
		Ports() : gate(0), peer(0) {};
		string stringify() { return to_string(ntohs(gate)) + ":" + to_string(ntohs(peer)); };
		Ports static extract(Connection * conn) {
			char * raw = conn->read(4);
			unsigned short * ports = (unsigned short *)(raw + 1);
			Ports result = {
					ports[0],
					ports[1]
			};

			delete raw; //Also deletes shared "ports"
			return result;
		}
};
#endif
