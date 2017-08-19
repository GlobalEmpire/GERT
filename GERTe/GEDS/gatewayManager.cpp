#include <sys/socket.h>
#include <fcntl.h>
#include "libLoad.h"
#include "gatewayManager.h"
#include "netty.h"
#include "routeManager.h"

map<Address, Gateway*> gateways; //Create Gateway database
vector<Gateway*> noAddrList; //Create list for unregistered gateways

//For Gateway iterators
bool gatewayIter::isEnd() { return ptr == gateways.end(); } //Add logic to isEnd()     Is last element
gatewayIter gatewayIter::operator++ (int a) { return (ptr++, *this); } //Add logic for ++ operator    Next element
gatewayIter::gatewayIter() : ptr(gateways.begin()) {}; //Add logic for constructor    First element
Gateway* gatewayIter::operator* () { return ptr->second; } //Add logic for * operator    This element

//See above its a mirror for non-registered gateways
bool noAddrIter::isEnd() { return ptr == noAddrList.end(); }
noAddrIter noAddrIter::operator++ (int a) { return (ptr++, *this); }
noAddrIter::noAddrIter() : ptr(noAddrList.begin()) {};
Gateway* noAddrIter::operator* () { return *ptr; }
void noAddrIter::erase() { ptr = noAddrList.erase(ptr); } //Add logic to erase()     Remove this element

Gateway* getGate(Address target) { //Get a Gateway from an address
	return gateways[target];
}

extern "C" {
	bool assign(Gateway* requestee, Address requested, Key key) { //Assign an address to a Gateway
		if (checkKey(requested, key)) { //Determine if the key is for the address
			requestee->addr = requested; //Set the address
			gateways[requested] = requestee; //Add Gateway to the database
			for (noAddrIter iter; !iter.isEnd(); iter++) { //Loop through list of non-registered gateways
				if (*iter == requestee) { //If we found the Gateway
					iter.erase(); //Remove it
					break; //We're done here
				}
			}
			log("Association from " + requested.stringify()); //Notify user that the address has registered
			return true; //Notify the protocol library assignment was successful
		}
		return false; //Notify the protocol library assignment has failed
	}
}

bool sendToGateway(Address addr, string data) { //Send to Gateway with address
	if (gateways.count(addr) != 0) { //If that Gateway is in the database
		sendByGateway(gateways[addr], data); //Send to that Gateway with Gateway
		return true; //Notify the protocol library that we succeeded in sending
	}
	return remoteSend(addr, data); //Attempt to send to Gateway with address via routing. Notify protocol library of result.
}

void closeGateway(Gateway* target) { //Close a full connection
	gateways.erase(target->addr); //Remove connection from universal map
	destroy((SOCKET*)target->sock); //Close the socket
	delete target; //Release the memory used to store the Gateway
	log("Disassociation from " + target->addr.stringify()); //Notify the user of the closure
}

void gateWatcher() { //Monitors gateways and their connections
	for (gatewayPtr iter = gateways.begin(); iter != gateways.end(); iter++) { //For every gatway in the database
		Gateway* target = iter->second; //Get the Gateway
		if (target == nullptr || target->sock == nullptr) { //If the Gateway or it's socket is broken
			iter = gateways.erase(iter); //Remove it from the database
			error("Null pointer in Gateway map"); //Notify the user of the error
		} else if (recv(*(SOCKET*)(target->sock), nullptr, 0, MSG_PEEK) == 0 || errno == ECONNRESET) { //If a socket read of 0 length fails
			closeGateway(target); //Close the Gateway smoothly
		}
	}
}

bool isLocal(Address addr) { //Check is an address is locally connected
	return gateways.count(addr) > 0;
}
