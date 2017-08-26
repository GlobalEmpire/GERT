#ifndef __ROUTEMANAGER__
#define __ROUTEMANAGER__
#include "netDefs.h"
#include <map>
using namespace std;

typedef map<Address, Peer*>::iterator routePtr;

class routeIter {
	routePtr ptr;
	public:
		bool isEnd();
		routeIter operator++(int);
		routeIter();
		routePtr operator->();
};

extern "C" {
	void killAssociated(Peer*);
	void setRoute(Address, Peer*);
	void removeRoute(Address);
	bool remoteSend(Address, string);
	bool isRemote(Address);
}
#endif
