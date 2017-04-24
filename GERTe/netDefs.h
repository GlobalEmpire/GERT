#ifdef _WIN32
#include <IPHlpApi.h>
#else
#include <sys/socket.h>
#endif
#include "libLoad.h"
typedef unsigned short USHORT;
typedef unsigned char UCHAR;

struct portComplex {
	USHORT gate, peer;
};

class peerAddr {
	in_addr ip;
	portComplex ports;
};

struct GERTaddr {
	USHORT high, low;
};

class connection {
	protected:
		void* sock;
		version api;
		connection(void* socket, version vers) : sock(socket), api(vers) {};
	public:
		UCHAR state = 0;
};

class gateway : public connection {
	public:
		GERTaddr addr;
		void process(string data);
		void kill();
		gateway(void* socket, version vers);
};

class peer : public connection {
	public:
		in_addr addr;
		void process(string data);
		void kill();
		peer(void* socket, version vers, in_addr source);
};

struct knownPeer {
	in_addr addr;
	portComplex ports;
};
