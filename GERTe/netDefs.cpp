#include "libLoad.h"
#include <string>
typedef unsigned char UCHAR;
typedef unsigned short USHORT;

struct in_addr {
	UCHAR seg[4];
};

//CAN BE PUBLIC
struct portComplex {
	USHORT gate, peer;
};

//PUBLIC
class peerAddr {
	in_addr ip;
	portComplex ports;
};

//PUBLIC
struct GERTaddr {
	USHORT high = 0;
	USHORT low = 0;
};

//PUBLIC
class connection {
	protected:
		void* sock;
		version api;
		connection(void* socket, version vers) : sock(socket), api(vers) {};
	public:
		UCHAR state = 0;
};

//PUBLIC
class gateway: public connection {
	public:
		GERTaddr addr;
		void process(string data) { (void)(api.procGate)(*this, data); };
		void kill() { (void)(api.killGate)(*this); };
		gateway(void* socket, version vers) : connection(socket, vers) {};
};

//PUBLIC
class peer : public connection {
	public:
		in_addr addr;
		void process(string data) { (void)(api.procPeer)(*this, data); };
		void kill() { (void)(api.killPeer)(*this); };
		peer(void* socket, version vers, in_addr source) : connection(socket, vers), addr(source) {};
};

//PUBLIC
struct knownPeer {
	in_addr addr;
	portComplex ports;
};
