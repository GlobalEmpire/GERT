#define WIN32_LEAN_AND_MEAN
#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#endif

#include "netty.h"
#include "gatewayManager.h"
#include "logging.h"
#include <fcntl.h>
#include <map>
#include "Poll.h"
#include "API.h"

using namespace std;

extern map<Address, Key> resolutions;

extern Poll gatePoll;

extern Version ThisVers;

namespace Gate {
	enum class Commands : char {
		STATE,
		REGISTER,
		DATA,
		CLOSE
	};

	enum class States : char {
		FAILURE,
		CONNECTED,
		REGISTERED,
		CLOSED,
		SENT
	};

	enum class Errors : char {
		VERSION,
		BAD_KEY,
		REGISTERED,
		NOT_REGISTERED,
		NO_ROUTE,
		ADDRESS_TAKEN
	};
}

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

#ifdef _WIN32
	ioctlsocket(*newSocket, FIONBIO, &nonZero);
#else
	int flags = fcntl(*newSocket, F_GETFL);
	fcntl(*newSocket, F_SETFL, flags | O_NONBLOCK);
#endif

	recv(*newSocket, buf, 3, 0); //Read first 3 bytes, the version data requested by gateway
	log((string)"Gateway using v" + to_string(buf[0]) + "." + to_string(buf[1]) + "." + to_string(buf[2])); //Notify user of connection and version
	UCHAR major = buf[0]; //Major version number
	if (major != ThisVers.vers.major) { //If the protocol library doesn't exist
		char error[3] = { 0, 0, 0 }; //Construct the error code
		send(*newSocket, error, 3, 0); //Notify client we cannot serve this version
		destroy(newSocket); //Close the socket
		warn("Gateway failed to connect");
		throw 1;
	} else {
		local = true;
		noAddrList.push_back(this);
		processGateway(this); //Process empty data (Protocol Library Gateway Initialization)
	}
}

Gateway::~Gateway() {
	if (local) {
		noAddrIter pos = find(this);
		pos.erase();
	}
	else {
		gateways.erase(this->addr); //Remove connection from universal map
		log("Disassociation from " + this->addr.stringify()); //Notify the user of the closure
	}

	gatePoll.remove(*(SOCKET*)sock);

	if (this->sock != nullptr)
		destroy((SOCKET*)this->sock); //Close the socket
}

Gateway* Gateway::lookup(Address req) {
	if (gateways.count(req) == 0) {
		throw 0;
	}

	return gateways[req];
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
	this->transmit(string({ (char)Gate::Commands::CLOSE })); //SEND CLOSE REQUEST
	this->transmit(string({ (char)Gate::Commands::STATE, (char)Gate::States::CLOSED })); //SEND STATE UPDATE TO CLOSED (0, 3)
	delete this; //Release the memory used to store the Gateway
}
