#include <sys/socket.h>
#include <fcntl.h>
#include "libLoad.h"
#include "gatewayManager.h"
#include "netty.h"
#include "routeManager.h"

map<GERTaddr, gateway*> gateways; //Create gateway database
vector<gateway*> noAddrList; //Create list for unregistered gateways

//For gateway iterators
bool gatewayIter::isEnd() { return ptr == gateways.end(); } //Add logic to isEnd()     Is last element
gatewayIter gatewayIter::operator++ (int a) { return (ptr++, *this); } //Add logic for ++ operator    Next element
gatewayIter::gatewayIter() : ptr(gateways.begin()) {}; //Add logic for constructor    First element
gateway* gatewayIter::operator* () { return ptr->second; } //Add logic for * operator    This element

//See above its a mirror for non-registered gateways
bool noAddrIter::isEnd() { return ptr == noAddrList.end(); }
noAddrIter noAddrIter::operator++ (int a) { return (ptr++, *this); }
noAddrIter::noAddrIter() : ptr(noAddrList.begin()) {};
gateway* noAddrIter::operator* () { return *ptr; }
void noAddrIter::erase() { ptr = noAddrList.erase(ptr); } //Add logic to erase()     Remove this element

gateway* getGate(GERTaddr target) { //Get a gateway from an address
	return gateways[target];
}

bool assign(gateway* requestee, GERTaddr requested, GERTkey key) { //Assign an address to a gateway
	if (checkKey(requested, key)) { //Determine if the key is for the address
		requestee->addr = requested; //Set the address
		gateways[requested] = requestee; //Add gateway to the database
		for (noAddrIter iter; !iter.isEnd(); iter++) { //Loop through list of non-registered gateways
			if (*iter == requestee) { //If we found the gateway
				iter.erase(); //Remove it
				break; //We're done here
			}
		}
		log("Association from " + requested.stringify()); //Notify user that the address has registered
		return true; //Notify the protocol library assignment was successful
	}
	return false; //Notify the protocol library assignment has failed
}

bool sendToGateway(GERTaddr addr, string data) { //Send to gateway with address
	if (gateways.count(addr) != 0) { //If that gateway is in the database
		sendByGateway(gateways[addr], data); //Send to that gateway with gateway
		return true; //Notify the protocol library that we succeeded in sending
	}
	return remoteSend(addr, data); //Attempt to send to gateway with address via routing. Notify protocol library of result.
}

void initGate(void * newSock) { //Create gateway with socket
	SOCKET * newSocket = (SOCKET*)newSock; //Convert socket to correct type
	char buf[3]; //Create a buffer for the version data
	recv(*newSocket, buf, 3, 0); //Read first 3 bytes, the version data requested by gateway
	log((string)"Gateway using " + to_string(buf[0]) + "." + to_string(buf[1]) + "." + to_string(buf[2])); //Notify user of connection and version
	UCHAR major = buf[0]; //Major version number
	version* api = getVersion(major); //Get the protocol library for the major version
#ifdef _WIN32 //If we're compiled in Windows
	ioctlsocket(*newSocket, FIONBIO, &nonZero); //Make socket non-blocking
#else //If we're compiled in *nix
	fcntl(*newSocket, F_SETFL, O_NONBLOCK); //Make socket non-blocking
#endif
	if (api == nullptr) { //If the protocol library doesn't exist
		char error[3] = { 0, 0, 0 }; //Construct the error code
		send(*newSocket, error, 3, 0); //Notify client we cannot serve this version
		destroy(newSocket); //Close the socket
	}
	else { //Major version found
		gateway * newConnection = new gateway(newSocket, api); //Create a new gateway object using the socket and protocol library
		noAddrList.push_back(newConnection); //Add new gateway to the unregistered list
		newConnection->process(""); //Process empty data (Protocol Library Gateway Initialization)
	}
}

void closeGateway(gateway* target) { //Close a full connection
	gateways.erase(target->addr); //Remove connection from universal map
	destroy((SOCKET*)target->sock); //Close the socket
	delete target; //Release the memory used to store the gateway
	log("Disassociation from " + target->addr.stringify()); //Notify the user of the closure
}

void gateWatcher() { //Monitors gateways and their connections
	for (gatewayPtr iter = gateways.begin(); iter != gateways.end(); iter++) { //For every gatway in the database
		gateway* target = iter->second; //Get the gateway
		if (target == nullptr || target->sock == nullptr) { //If the gateway or it's socket is broken
			iter = gateways.erase(iter); //Remove it from the database
			error("Null pointer in gateway map"); //Notify the user of the error
		} else if (recv(*(SOCKET*)(target->sock), nullptr, 0, MSG_PEEK) == 0 || errno == ECONNRESET) { //If a socket read of 0 length fails
			closeGateway(target); //Close the gateway smoothly
		}
	}
}

bool isLocal(GERTaddr addr) { //Check is an address is locally connected
	return gateways.count(addr) > 0;
}
