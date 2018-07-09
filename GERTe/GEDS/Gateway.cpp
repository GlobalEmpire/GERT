#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#endif

#include "libLoad.h"
#include "netty.h"
#include "gatewayManager.h"
#include "logging.h"
#include <fcntl.h>
#include <map>
#include "Poll.h"

typedef int SOCKET;

extern map<Address, Key> resolutions;

extern Poll gatePoll;

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
		warn("Gateway failed to connect");
		throw 1;
	} else {
		local = true;
		noAddrList.push_back(this);
		this->process(); //Process empty data (Protocol Library Gateway Initialization)
	}
}

Gateway* Gateway::lookup(Address req) {
	if (gateways.count(req) == 0) {
		throw 0;
	}

	return gateways[req];
}

/*Gateway::~Gateway() {

}*/

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
	noAddrIter pos = find(this);
	if (!pos.isEnd()) {
		pos.erase();
	} else {
		gateways.erase(this->addr); //Remove connection from universal map
		log("Disassociation from " + this->addr.stringify()); //Notify the user of the closure
	}

	gatePoll.remove(*(SOCKET*)sock);

	if (this->sock != nullptr)
		destroy((SOCKET*)this->sock); //Close the socket
	delete this; //Release the memory used to store the Gateway
}
