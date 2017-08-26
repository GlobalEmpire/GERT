#ifndef __NETDEFS__
#define __NETDEFS__
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "libDefs.h"
#include "logging.h"
#include "GERTc.h"

using namespace std;

typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;

class portComplex {
	public:
		USHORT gate, peer;
		portComplex(USHORT gatePort, USHORT peerPort) : gate(htons(gatePort)), peer(htons(peerPort)) {};
		portComplex() : gate(0), peer(0) {};
		string stringify() { return to_string(ntohs(gate)) + ":" + to_string(ntohs(peer)); };
};

class ipAddr {
	public:
		in_addr addr;
		bool operator< (ipAddr comp) const { return (addr.s_addr < comp.addr.s_addr); };
		bool operator== (ipAddr comp) const { return (addr.s_addr == comp.addr.s_addr); };
		ipAddr(unsigned long target) : addr(*(in_addr*)&target) {};
		ipAddr(in_addr target) : addr(target) {};
		ipAddr(string target) : addr(*(in_addr*)target.c_str()) {};
		string stringify() {
			UCHAR* rep = (UCHAR*)&addr.s_addr;
			return to_string(rep[0]) + "." + to_string(rep[1]) + "." + to_string(rep[2]) + "." + to_string(rep[3]);
		};
};

class connection {
	public:
		void* sock;
		version* api;
		UCHAR state = 0;
		connection(void* socket) : sock(socket), api(nullptr) {};
		connection(void* socket, version* vers) : sock(socket), api(vers) {};
};
#endif
