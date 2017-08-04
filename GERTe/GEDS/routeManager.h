#ifndef __ROUTEMANAGER__
#define __ROUTEMANAGER__
#include "netDefs.h"
#include <map>
using namespace std;

typedef map<GERTaddr, peer*>::iterator routePtr;

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
	void setRoute(GERTaddr, peer*);
	void removeRoute(GERTaddr);
	bool remoteSend(GERTaddr, string);
	bool isRemote(GERTaddr);
}
#endif
