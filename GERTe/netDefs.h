#ifndef __NETDEFS__
#define __NETDEFS__
#ifdef _WIN32
#include <IPHlpApi.h>
#else
#include <netinet/ip.h>
#include <arpa/inet.h>
#endif
#include <string>
#include "libDefs.h"
#include "logging.h"

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
		ipAddr(UCHAR target[4]) {
			string ip = to_string(target[0]) + "." + to_string(target[1]) + "." + to_string(target[2]) + "." + to_string(target[3]);
			log(ip);
			log(to_string(inet_aton(ip.c_str(), &addr)));
		};
		string stringify() {
			UCHAR* rep = (UCHAR*)&addr.s_addr;
			return to_string(rep[0]) + "." + to_string(rep[1]) + "." + to_string(rep[2]) + "." + to_string(rep[3]);
		};
};

class GERTaddr {
	public:
		USHORT high, low;
		bool operator < (const GERTaddr comp) const { return (high < comp.high or (high == comp.high and low < comp.low)); };
		bool operator == (const GERTaddr comp) const { return (high == comp.high and low == comp.low); };
		string stringify() { return to_string(high) + to_string(low); };
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
		peer(void* mysocket, version* vers, in_addr source) : connection(mysocket, vers), addr(source) {};
};

class knownPeer {
	public:
		ipAddr addr;
		portComplex ports;
		knownPeer(ipAddr target, portComplex pair) : addr(target), ports(pair) {};
		knownPeer() : addr((UCHAR*)"\0\0\0\0"), ports((USHORT)0, (USHORT)0) {};
};
#endif
