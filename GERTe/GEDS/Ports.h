#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <netinet/ip.h>
#endif

#include "Connection.h"

class Ports {
	public:
		unsigned short gate, peer;
		Ports(unsigned short gatePort, unsigned short peerPort) : gate(gatePort), peer(peerPort) {};
		Ports(Connection * conn) {
			char * raw = conn->read(4);
			unsigned short * ports = (unsigned short *)(raw + 1);

			gate = ports[0];
			peer = ports[1];

			delete raw;
		}
		Ports() : gate(0), peer(0) {};
		std::string stringify() { return std::to_string(ntohs(gate)) + ":" + std::to_string(ntohs(peer)); };
};