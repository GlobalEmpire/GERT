#ifndef __ROUTEMANAGER__
#define __ROUTEMANAGER__
#include "netDefs.h"
#include <map>
using namespace std;

typedef map<Address, peer*>::iterator routePtr;

class routeIter {
	routePtr ptr;
	public:
		bool isEnd();
		routeIter operator++(int);
		routeIter();
		routePtr operator->();
};

extern "C" {
	void killAssociated(peer*);
	void setRoute(Address, peer*);
	void removeRoute(Address);
	bool remoteSend(Address, string);
	bool isRemote(Address);
}
#endif
