/* 
	This is the primary code for the GEDS server.
	This file implements platform independent definitions for internet sockets.
	Supported platforms are currently Linux(/Unix) and Windows
	To modify the default ports change the numbers in the config.h file.
	
	This code is designed to function with other code from
	https://github.com/GlobalEmpire/GERT/
	
	This code falls under the license located at
	https://github.com/GlobalEmpire/GERT/blob/master/License.md
 */

#define WIN32_LEAN_AND_MEAN

typedef unsigned char UCHAR; //Creates UCHAR shortcut for Unsigned Character
typedef unsigned short ushort;
typedef map<gateway, connection<gateway>>::iterator iter1;
typedef map<gateway, connection<peer>>::iterator iter2;

#ifdef _WIN32
#include <Windows.h>
#include <ws2tcpip.h> //Include windows TCP API
#include <iphlpapi.h> //Include windows IP helper API
#include <winsock2.h> //Include Winsock v2
#else
#include <socket.h>
#endif

#include <thread> //Include thread type
#include <string> //Include string type
#include <map> //Include map type
#include <signal.h> //Include signal processing API
#include <stdint.h>
#include "netDefs.h"
#include <iostream>
#include "netty.h"
#include "keyMngr.h"
using namespace std; //Default namespace to std

enum status {
	NORMAL,
	NO_LIBS,
	LIB_LOAD_ERR
};

map<in_addr, connection<peer>> peers; //Create lookup for IPv4 address to conneciton mapping for GEDS P2P
map<gateway, GERTkey> resolutions; //Create name resolution lookup table

volatile bool running = true; //SIGINT tracker

void shutdownProceedure(int param) { //SIGINT handler function
	running = false; //Flip tracker to disable threads and trigger main thread's cleanup
};

//MARKED FOR UPDATING
void closeConnection(connection target) { //Close a full connection
	lookup.erase(*target.socket); //Remove connection from universal map
	if (target.type == GATEWAY) //If connection is a gateway
		gateway gate = *(gateway*)target.ptr;
	else {//If connection is a GEDS server
		
		peers[target.ip] = empty;
	}
	closeSock(*target.socket); //Close the socket
}

void sendTo(GERTaddr addr, string data) {
	gateway target = gateways[addr];
	send(*target.socket, data.c_str(), data.length, NULL);
}

void sendTo(connection conn, string data) {
	send(*conn.socket, data.c_str(), data.length, NULL);
}

void process() {
	while (running) {
		sockiter iter = lookup.begin();
		sockiter end = lookup.end();
		for (sockiter iter = lookup.begin(); iter != lookup.end(); iter++) {
			char buf[256];
			SOCKET sock = iter->first;
			connection conn = iter->second;
			int result = recv(sock, buf, 256, NULL);
			if (result = 0)
				continue;
			else if (conn.type == GATEWAY)
				conn.api.processGateway(*(gateway*)conn.ptr, buf);
			else if (conn.type == GEDS)
				conn.process(*(geds*)conn.ptr, buf);
		}
		this_thread::yield();
	}
}

void loadResolutions() {
	FILE* resolutionFile = fopen("resolutions.geds", "rb");
	while (true) {
		GERTaddr addr;
		fread(&addr, 2, 2, resolutionFile);
		GERTkey key;
		fread(&addr, 20, 1, resolutionFile);
		cout << "Imported resolution for " << to_string(addr.high) << "-" << to_string(addr.low) << "\n";
		addResolution(addr, key);
		if (feof(resolutionFile) != 0)
			break;
	}
	fclose(resolutionFile);
}

string readableIP(in_addr ip) {
	UCHAR * seg = (UCHAR*)(void*)&ip;
	string high = to_string(seg[0]) + "." + to_string(seg[1]);
	string low = to_string(seg[2]) + "." + to_string(seg[3]);
	return high + "." + low;
}

void loadPeers() {
	FILE* peerFile = fopen("peers.geds", "rb");
	while (true) {
		in_addr ip;
		fread(&ip, 4, 1, peerFile); //Why must I choose between 1, 4 and 4, 1? Or 2, 2?
		portComplex ports;
		fread(&ports, 2, 2, peerFile);
		cout << "Imported peer " << readableIP(ip) << ":" << ports.gate << ":" << ports.peer << "\n";
		addPeer(ip, ports);
		if (feof(peerFile) != 0)
			break;
	}
	fclose(peerFile);
}

int main() {

	int libErr = loadLibs(); //Load gelib files

	switch (libErr) { //Test for errors loading libraries
		case EMPTY:
			return NO_LIBS;
		case UNKNOWN:
			return LIB_LOAD_ERR;
	}

	startup(); //Startup servers

	//System processing
	signal(SIGINT, &shutdownProceedure); //Hook SIGINT with custom handler
	thread processor(process);
	runServer();

	//Shutdown and Cleanup sequence
	processor.join(); //Cleanup processor (wait for it to die)
	killConnections(); //Kill any remaining connections

	shutdown();//Cleanup servers
	
	return NORMAL; //Return with exit state "NORMAL" (0)
}
