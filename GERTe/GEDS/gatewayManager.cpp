#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#endif

#include <fcntl.h>
#include "gatewayManager.h"
#include "netty.h"
#include "routeManager.h"
#include "logging.h"

using namespace std;

extern map<Address, Gateway*> gateways; //Create Gateway database
extern vector<Gateway*> noAddrList; //Create list for unregistered gateways

//For Gateway iterators
bool gatewayIter::isEnd() { return ptr == gateways.end(); } //Add logic to isEnd()     Is last element
gatewayIter gatewayIter::operator++ (int a) { if (!this->isEnd()) ptr++; return *this;} //Add logic for ++ operator    Next element
gatewayIter::gatewayIter() : ptr(gateways.begin()) {}; //Add logic for constructor    First element
Gateway* gatewayIter::operator* () { return ptr->second; } //Add logic for * operator    This element
void gatewayIter::erase() { ptr = gateways.erase(ptr); }

//See above its a mirror for non-registered gateways
bool noAddrIter::isEnd() { return ptr >= noAddrList.end(); }
noAddrIter noAddrIter::operator++ (int a) { return (ptr++, *this); }
noAddrIter::noAddrIter() : ptr(noAddrList.begin()) {};
Gateway* noAddrIter::operator* () { return *ptr; }
void noAddrIter::erase() { ptr = noAddrList.erase(ptr); } //Add logic to erase()     Remove this element

bool sendToGateway(Address addr, string data) { //Send to Gateway with address
	if (gateways.count(addr) != 0) { //If that Gateway is in the database
		gateways[addr]->transmit(data);
		return true; //Notify the protocol library that we succeeded in sending
	}
	return remoteSend(addr, data); //Attempt to send to Gateway with address via routing. Notify protocol library of result.
}

void gateWatcher() { //Monitors gateways and their connections
	for (gatewayIter iter; !iter.isEnd(); iter++) { //For every gatway in the database
		Gateway* target = *iter; //Get the Gateway
		if (target == nullptr || target->sock == nullptr) { //If the Gateway or it's socket is broken
			error("Null pointer in Gateway map"); //Notify the user of the error
			iter.erase(); //Remove it from the database
		} else if (recv(*(SOCKET*)(target->sock), nullptr, 0, MSG_PEEK) == 0 || errno == ECONNRESET) { //If a socket read of 0 length fails
			target->close(); //Close the Gateway smoothly
		}
	}
}

void noAddrWatcher() {
	for (noAddrIter iter; !iter.isEnd(); iter++) {
		Gateway* target = *iter;
		if (target == nullptr || target->sock == nullptr) {
			iter.erase();
			error("Null pointer in Gateway list");
		} else if (recv(*(SOCKET*)(target->sock), nullptr, 0, MSG_PEEK) == 0 || errno == ECONNRESET) {
			destroy((SOCKET*)target->sock);
			delete target;
			warn("Unassociated gateway was lost");
		}
	}
}
