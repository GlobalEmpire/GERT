#include "Gateway.h"
#include "libLoad.h"
#include "netty.h"
#include "gatewayManager.h"
#include "logging.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <map>

typedef int SOCKET;

extern map<Address, Key> resolutions;

map<Address, Gateway*> gateways;
vector<Gateway*> noAddrList;

noAddrIter find(Gateway * gate) {
	noAddrIter iter;
	for (iter; !iter.isEnd(); iter++) { //Loop through list of non-registered gateways
		if (*iter == gate) { //If we found the Gateway
			return iter;
		}
	}
	return iter;
}

Gateway::Gateway(void* sock) : Connection(sock) {
	SOCKET * newSocket = (SOCKET*)sock; //Convert socket to correct type
	char buf[3]; //Create a buffer for the version data
	recv(*newSocket, buf, 3, 0); //Read first 3 bytes, the version data requested by gateway
	log((string)"Gateway using v" + to_string(buf[0]) + "." + to_string(buf[1]) + "." + to_string(buf[2])); //Notify user of connection and version
	UCHAR major = buf[0]; //Major version number
	api = getVersion(major); //Get the protocol library for the major version
#ifdef _WIN32 //If we're compiled in Windows
	ioctlsocket(*newSocket, FIONBIO, &nonZero); //Make socket non-blocking
#else //If we're compiled in *nix
	fcntl(*newSocket, F_SETFL, O_NONBLOCK); //Make socket non-blocking
#endif
	if (api == nullptr) { //If the protocol library doesn't exist
		char error[3] = { 0, 0, 0 }; //Construct the error code
		send(*newSocket, error, 3, 0); //Notify client we cannot serve this version
		destroy(newSocket); //Close the socket
		delete newSocket;
		delete this;
		throw;
	} else {
		this->process(); //Process empty data (Protocol Library Gateway Initialization)
		noAddrList.push_back(this);
	}
}

void Gateway::transmit(string data) {
	send(*(SOCKET*)this->sock, data.c_str(), (ULONG)data.length(), 0);
}

bool Gateway::assign(Address requested, Key key) {
	if (resolutions[requested] == key) { //Determine if the key is for the address
		this->addr = requested; //Set the address
		gateways[requested] = this; //Add Gateway to the database
		noAddrIter pos = find(this);
		pos.erase();
		log("Association from " + requested.stringify()); //Notify user that the address has registered
		return true; //Notify the protocol library assignment was successful
	}
	return false; //Notify the protocol library assignment has failed
}

void Gateway::close() {
	gateways.erase(this->addr); //Remove connection from universal map
	noAddrIter pos = find(this);
	if (!pos.isEnd())
		pos.erase();
	if (this->sock != nullptr)
		destroy((SOCKET*)this->sock); //Close the socket
	log("Disassociation from " + this->addr.stringify()); //Notify the user of the closure
	delete this; //Release the memory used to store the Gateway
}
