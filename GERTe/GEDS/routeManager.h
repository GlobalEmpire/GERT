#ifndef __ROUTEMANAGER__
#define __ROUTEMANAGER__
#include <map>

typedef map<GERTaddr, peer*>::iterator routePtr;

class routeIter {
	routePtr ptr;
	public:
		bool isEnd();
		routeIter operator++(int);
		routeIter();
		routePtr operator->();
};

void killAssociated(peer*);
void setRoute(GERTaddr, peer*);
void removeRoute(GERTaddr);
bool remoteSend(GERTaddr, string);
#endif
