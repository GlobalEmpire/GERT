#include "libLoad"
typedef unsigned short USHORT;
typedef unsigned char UCHAR;

struct portComplex {
	USHORT gate, peer;
}

class peer {
	in_addr ip;
	portComplex ports;
}

struct gateway {
	USHORT high, low;
}

template<class STATE>
class connection {
	SOCKET * sock;
	public:
		STATE info;
		UCHAR state;
		version api;
}

template<>
connection::connection(SOCKET*, version)<gateway>;

template<>
connection::connection(SOCKET*, version, peer)<peer>;
