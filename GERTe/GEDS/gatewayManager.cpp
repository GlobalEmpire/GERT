#include "netDefs.h"
#include "keyMngr.h"
#include <sys/socket.h>
#include <fcntl.h>
#include "libLoad.h"
#include "gatewayManager.h"
#include "netty.h"
#include "routeManager.h"

map<GERTaddr, gateway*> gateways;
vector<gateway*> noAddrList;

bool gatewayIter::isEnd() { return ptr == gateways.end(); }
gatewayIter gatewayIter::operator++ (int a) { return (ptr++, *this); }
gatewayIter::gatewayIter() : ptr(gateways.begin()) {};
gateway* gatewayIter::operator* () { return ptr->second; }

bool noAddrIter::isEnd() { return ptr == noAddrList.end(); }
noAddrIter noAddrIter::operator++ (int a) { return (ptr++, *this); }
noAddrIter::noAddrIter() : ptr(noAddrList.begin()) {};
gateway* noAddrIter::operator* () { return *ptr; }
void noAddrIter::erase() { ptr = noAddrList.erase(ptr); }

gateway* getGate(GERTaddr target) {
	return gateways[target];
}

bool assign(gateway* requestee, GERTaddr requested, GERTkey key) {
	if (checkKey(requested, key)) {
		requestee->addr = requested;
		gateways[requested] = requestee;
		for (noAddrIter iter; !iter.isEnd(); iter++) {
			if (*iter == requestee) {
				iter.erase();
				break;
			}
		}
		log("Association from " + requested.stringify());
		return true;
	}
	return false;
}

bool sendTo(GERTaddr addr, string data) {
	if (gateways.count(addr) != 0) {
		sendTo(gateways[addr], data);
		return true;
	}
	return remoteSend(addr, data);
}

void initGate(void * newSock) {
	SOCKET * newSocket = (SOCKET*)newSock;
#ifdef _WIN32
	ioctlsocket(*newSocket, FIONBIO, &nonZero);
#else
	fcntl(*newSocket, F_SETFL, O_NONBLOCK);
#endif
	char buf[3];
	recv(*newSocket, buf, 3, 0); //Read first 3 bytes, the version data requested by gateway
	log((string)"Gateway using " + buf[0] + "." + buf[1] + "." + buf[2]);
	UCHAR major = buf[0]; //Major version number
	version* api = getVersion(major);
	if (api == nullptr) {
		char error[3] = { 0, 0, 0 };
		send(*newSocket, error, 3, 0); //Notify client we cannot serve this version
		destroy(newSocket); //Close the socket
	}
	else { //Major version found
		gateway * newConnection = new gateway(newSocket, api);
		noAddrList.push_back(newConnection);
		newConnection->process("");
	}
}

void closeTarget(gateway* target) { //Close a full connection
	gateways.erase(target->addr); //Remove connection from universal map
	destroy((SOCKET*)target->sock); //Close the socket
	delete target;
	log("Disassociation from " + target->addr.stringify());
}

void gateWatcher(){
	for (gatewayPtr iter = gateways.begin(); iter != gateways.end(); iter++) {
		gateway* target = iter->second;
		if (target == nullptr || target->sock == nullptr) {
			iter = gateways.erase(iter);
			error("Null pointer in gateway map");
			continue;
		}
		if (recv(*(SOCKET*)(target->sock), nullptr, 0, MSG_PEEK) == 0 || errno == ECONNRESET) {
			closeTarget(target);
		}
	}
}
