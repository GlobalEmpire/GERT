#ifndef __NETDEFS__
#define __NETDEFS__
#include <netinet/ip.h>
#include <arpa/inet.h>
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
		ipAddr(unsigned long target) : addr(*(in_addr*)&target) {};
		ipAddr(in_addr target) : addr(target) {};
		ipAddr(string target) : addr(*(in_addr*)target.c_str()) {};
		string stringify() {
			UCHAR* rep = (UCHAR*)&addr.s_addr;
			return to_string(rep[0]) + "." + to_string(rep[1]) + "." + to_string(rep[2]) + "." + to_string(rep[3]);
		};
};

class GERTaddr {
	public:
		UCHAR eAddr[3];
		UCHAR iAddr[3];
		bool operator < (const GERTaddr comp) const { return (eAddr < comp.eAddr || (eAddr == comp.eAddr && iAddr < comp.iAddr)); };
		bool operator == (const GERTaddr comp) const { return (eAddr == comp.eAddr && iAddr == comp.iAddr); };
		GERTaddr(UCHAR* e, UCHAR* i) : eAddr{e[0], e[1], e[2]}, iAddr{i[0], i[1], i[2]} {};
		GERTaddr(UCHAR* e) : eAddr{e[0], e[1], e[2]}, iAddr{0, 0, 0} {};
		GERTaddr() : eAddr{0, 0, 0}, iAddr{0, 0, 0} {};
		string stringify() {
			USHORT eHigh, eLow;
			USHORT iHigh, iLow;
			eHigh = (USHORT)(eAddr[0]) << 4 | (USHORT)(eAddr[1]) >> 4;
			eLow = ((USHORT)eAddr[1] & 0x0F) << 8 | (USHORT)eAddr[2];
			iHigh = (USHORT)iAddr[0] << 4 | (USHORT)iAddr[1] >> 4;
			iLow = ((USHORT)iAddr[1] & 0x0F) << 8 | (USHORT)iAddr[2];
			/*eHigh = ntohs(eHigh);
			eLow = ntohs(eLow);
			iHigh = ntohs(iHigh);
			iLow = ntohs(iLow);*/
			string eString = to_string(eHigh) + "." + to_string(eLow);
			string iString = to_string(iHigh) + "." + to_string(iLow);
			return eString + "." + iString;
		};
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
		knownPeer() : addr(0L), ports((USHORT)0, (USHORT)0) {};
};
#endif
