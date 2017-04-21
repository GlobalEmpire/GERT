#include "libLoad"
typedef unsigned char UCHAR;
typedef unsigned short USHORT;

//CAN BE PUBLIC
struct portComplex {
	USHORT gate, peer;
}

//PUBLIC
class peer {
	in_addr ip;
	portComplex ports;
}

//PUBLIC
struct gateway {
	USHORT high, low;
}

//PUBLIC
template<class STATE>
class connection {
	void* sock;
	public:
		STATE info;
		UCHAR state;
		version api;
}

//PUBLIC
template<>
connection::connection(void* sock, version vers)<gateway> : socket(sock), api(vers) { //Create connection constructor
	info.state = 0;
	gateway addr;
	addr.high = 0;
	addr.low = 0;
	info = addr;
}

//PUBLIC
template<>
connection::connection(void* sock, version vers, peer geds)<peer> : socket(sock), api(vers), info(geds), state(0) {}

peer::peer(remote, gate, geds) ip(remote) {
	portComplex pair;
	pair.gate = gate;
	pair.peer = geds;
	ports = pair;
}


