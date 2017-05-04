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

typedef unsigned char UCHAR; //Creates UCHAR shortcut for Unsigned Character
typedef unsigned short ushort;

#ifdef _WIN32
#include <iphlpapi.h>
#else
#include <netinet/ip.h>
#endif

#include <thread> //Include thread type
#include <string> //Include string type
#include <signal.h> //Include signal processing API
#include <iostream>
#include "netty.h"
#include "libLoad.h"
#include "logging.h"
#include <exception>
using namespace std; //Default namespace to std

enum status {
	NORMAL,
	NO_LIBS,
	LIB_LOAD_ERR,
	PEER_LOAD_ERR,
	KEY_LOAD_ERR,
	UNKNOWN_CRITICAL
};

volatile bool running = false; //SIGINT tracker

void shutdownProceedure(int param) { //SIGINT handler function
	warn("Killing program.");
	running = false; //Flip tracker to disable threads and trigger main thread's cleanup
};

void OHCRAPOHCRAP(int param) {
	cout << "Well, this is the end\n";
	cout << "I've read too much, and written so far\n";
	cout << "But here I am, faulting to death\n";
	cout << "\n\nSIGSEGV THROWN: SEGMENTATION FAULT. GOODBYE\n";
	abort();
}

void errHandler() {
	cout << "CRTICIAL ERROR: ABNORMAL TERMINATION\n";
	exit(UNKNOWN_CRITICAL);
}

int loadResolutions() {
	FILE* resolutionFile = fopen("resolutions.geds", "rb");
	if (resolutionFile == nullptr)
		return KEY_LOAD_ERR;
	while (true) {
		USHORT buf[2];
		GERTaddr addr = {buf[0], buf[1]};
		fread(&buf, 2, 2, resolutionFile);
		if (feof(resolutionFile) != 0)
			break;
		char buff[20];
		fread(&buff, 1, 20, resolutionFile);
		GERTkey key(buff);
		log("Imported resolution for " + to_string(addr.high) + "-" + to_string(addr.low));
		addResolution(addr, key);
	}
	fclose(resolutionFile);
	return NORMAL;
}

int loadPeers() {
	FILE* peerFile = fopen("peers.geds", "rb");
	if (peerFile == nullptr)
		return PEER_LOAD_ERR;
	while (true) {
		UCHAR ip[4];
		fread(&ip, 1, 4, peerFile); //Why must I choose between 1, 4 and 4, 1? Or 2, 2?
		if (feof(peerFile) != 0)
			break;
		USHORT rawPorts[2];
		fread(&rawPorts, 2, 2, peerFile);
		portComplex ports = {rawPorts[0], rawPorts[1]};
		ipAddr ipClass = ip;
		log("Importing peer " + ipClass.stringify() + ":" + ports.stringify());
		addPeer(ipClass, ports);
	}
	fclose(peerFile);
	return NORMAL;
}

int main() {

	set_terminate(errHandler);

	int libErr = loadLibs(); //Load gelib files

	switch (libErr) { //Test for errors loading libraries
		case EMPTY:
			error("No libraries detected.");
			return NO_LIBS;
		case UNKNOWN:
			error("Unknown error occurred while loading libraries.");
			return LIB_LOAD_ERR;
	}

	int result = loadPeers();

	if (result != NORMAL)
		return result;

	int result2 = loadResolutions();

	if (result2 != NORMAL)
		return result2;

	startup(); //Startup servers
	buildWeb();

	running = true;

	//System processing
	signal(SIGINT, &shutdownProceedure); //Hook SIGINT with custom handler
	signal(SIGSEGV, &OHCRAPOHCRAP);
	thread processor(process);
	runServer();
	warn("Primary server killed.");

	//Shutdown and Cleanup sequence
	processor.join(); //Cleanup processor (wait for it to die)
	warn("Processor killed, program ending.");

	shutdown();//Cleanup servers
	
	return NORMAL; //Return with exit state "NORMAL" (0)
}
