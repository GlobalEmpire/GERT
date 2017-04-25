#ifndef __NETDEFS__
#define __NETDEFS__
#ifdef _WIN32
#include <IPHlpApi.h>
#else
#include <netinet/ip.h>
#endif
#include <string>
#include "libLoad.h"

using namespace std;

typedef unsigned short USHORT;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;

struct portComplex {
	USHORT gate, peer;
};

class peerAddr {
	in_addr ip;
	portComplex ports;
};

class ipAddr {
	public:
		in_addr addr;
		bool operator< (ipAddr comp) const { return (addr.s_addr < comp.addr.s_addr); };
		bool operator== (ipAddr comp) const { return (addr.s_addr == comp.addr.s_addr); };
		ipAddr(in_addr target) : addr(target) {};
		ipAddr(ULONG target) : addr(*(in_addr*)(&target)) {};
		ipAddr(char target[4]) : addr(*(in_addr*)(&target)) {};
};

class GERTaddr {
	public:
		USHORT high, low;
		bool operator < (const GERTaddr comp) const { return (high < comp.high or (high == comp.high and low < comp.low)); };
		bool operator == (const GERTaddr comp) const { return (high == comp.high and low == comp.low); };
};

class connection {
	public:
		void* sock;
		version* api;
		UCHAR state = 0;
		connection(void* socket, version* vers) : sock(socket), api(vers) {};
};

class gateway : public connection {
	public:
		GERTaddr addr;
		void process(string data) { api->procGate(this, data); };
		void kill() { api->killGate(this); };
		gateway(void* socket, version* vers) : connection(socket, vers) {};
};

class peer : public connection {
	public:
		ipAddr addr;
		void process(string data) { api->procPeer(this, data); };
		void kill() { api->killPeer(this); };
		peer(void* socket, version* vers, in_addr source) : connection(sock, vers), addr(source) {};
};

class knownPeer {
	public:
		ipAddr addr;
		portComplex ports;
		knownPeer(ipAddr target, portComplex pair) : addr(target), ports(pair) {};
		knownPeer() : addr("\0\0\0\0"), ports({0,0}) {};
};
#endif
