/* 
	This is the primary code for the GEDS server.
	This file implements platform independent definitions for internet sockets.
	Supported platforms are currently Linux(/Unix)
	To modify the default ports change the numbers in the config.h file.
	
	This code is designed to function with other code from
	https://github.com/GlobalEmpire/GERT/
	
	This code falls under the license located at
	https://github.com/GlobalEmpire/GERT/blob/master/License.md
 */

typedef unsigned char UCHAR; //Creates UCHAR shortcut for Unsigned Character
typedef unsigned short ushort;

#include <thread> //Include thread type
#include <signal.h> //Include signal processing API
#include <iostream>
#include "netty.h"
#include "libLoad.h"
#include "overwatch.h"
#include "fileMngr.h"
#include <exception>
using namespace std; //Default namespace to std

enum status {
	NORMAL,
	NO_LIBS,
	LIB_LOAD_ERR,
	PEER_LOAD_ERR,
	KEY_LOAD_ERR,
	UNKNOWN_CRITICAL,
	UNKNOWN_MINOR
};

volatile bool running = false; //SIGINT tracker
bool debugMode = false;

char * LOCAL_IP = nullptr;
char * peerPort = "59474";
char * gatewayPort = "43780";

void shutdownProceedure(int param) { //SIGINT handler function
	if (running) {
		warn("User requested shutdown. Flipping the switch!");
		running = false; //Flip tracker to disable threads and trigger main thread's cleanup
	} else {
		error("Server wasn't in running state when SIGINT was raised.");
		error("!!! Forcing termination of server. !!!");
		exit(UNKNOWN_MINOR);
	}
};

void OHCRAPOHCRAP(int param) {
	int errCode = emergencyScan();
	saveResolutions();
	if (errCode < 2) {
		savePeers();
		debug("Peers uncorrupted, peer file updated.");
	}
	cout << "Well, this is the end\n";
	cout << "I've read too much, and written so far\n";
	cout << "But here I am, faulting to death\n";
	cout << "\nSIGSEGV THROWN: SEGMENTATION FAULT.\n";
	cout << "SIGSEGV is a critical error thrown by the operating system\n";
	cout << "It signifies a MASSIVE error that won't correct itself and probably can't be fixed without changing the code.\n";
	cout << "Please post a bug report including any potentially useful logs.\n";
	exit(UNKNOWN_CRITICAL);
}

void errHandler() {
	int errCode = emergencyScan();
	saveResolutions();
	if (errCode < 2) {
		savePeers();
		debug("Peers uncorrupted, peer file updated.");
	}
	cout << "Unknown error, system called terminate() with code " << to_string(errno) << "\n";
	exit(UNKNOWN_CRITICAL);
}

void printHelp() {
	cout << "Requires atleast the -a parameter\n";
	cout << "-a publicIP   Specifies the public IP to prevent loops.\n";
	cout << "-p port       Specifies the port for GEDS servers to connect to (default 59474)\n";
	cout << "-g port       Specifies the port for Gateways to connect to (default 43780)\n";
	cout << "-h            Displays this message\n";
	cout << "-d            Prints out extra debug information\n";
	exit(0);
}

void processArgs(int argc, char* argv[]) {
	if (argc < 2) {
		printHelp();
	}
	for (int i = 1; i < argc; i++) {
		string curArg = argv[i];
		if (curArg == "-d") {
			debugMode = true;
		} else if (curArg == "-a") {
			if (argc == i)
				printHelp();
			LOCAL_IP = argv[++i];
		} else if (curArg  == "-p") {
			if (argc == i)
				printHelp();
			peerPort = argv[++i];
		} else if (curArg == "-g") {
			if (argc == i)
				printHelp();
			gatewayPort = argv[++i];
		} else if (curArg == "-h")
				printHelp();
	}
	if (LOCAL_IP == nullptr)
		printHelp();
}

int main( int argc, char* argv[] ) {

	processArgs(argc, argv);
	debug((string)"Processed arguments. Gateway port: " + gatewayPort + " Peer port: " + peerPort + " Local IP: " + LOCAL_IP + " Debug mode: " + to_string(debugMode) + " (should be 1)");

	startLog();

	set_terminate(errHandler);
	signal(SIGSEGV, &OHCRAPOHCRAP);
	signal(SIGINT, &shutdownProceedure); //Hook SIGINT with custom handler

	debug("Loading libraries");
	int libErr = loadLibs(); //Load gelib files

	switch (libErr) { //Test for errors loading libraries
		case EMPTY:
			error("No libraries detected.");
			return NO_LIBS;
		case UNKNOWN:
			error("Unknown error occurred while loading libraries.");
			return LIB_LOAD_ERR;
	}

	debug("Loading peers");
	int result = loadPeers();

	if (result != NORMAL) {
		error("Failed to load peers");
		return PEER_LOAD_ERR;
	}

	debug("Loading resolutions");
	int result2 = loadResolutions();

	if (result2 != NORMAL) {
		error("Failed to load keys");
		return KEY_LOAD_ERR;
	}

	debug("Starting servers");
	startup(); //Startup servers

	debug("Building peer web");
	buildWeb();

	running = true;

	debug("Starting overwatch garbage sorter");
	thread watcher(overwatch);

	debug("Starting message processor");
	thread processor(process);

	debug("Starting main server loop");
	runServer();
	warn("Primary server killed.");

	//Shutdown and Cleanup sequence
	debug("Waiting for message processor to exit");
	processor.join(); //Cleanup processor (wait for it to die)
	warn("Processor killed, program ending.");

	debug("Waiting for overwatch garbage sorter to exit");
	watcher.join();

	debug("Cleaning up servers");
	shutdown();//Cleanup servers
	
	debug("Saving peers");
	savePeers();

	debug("Saving resolutions");
	saveResolutions();

	stopLog();

	return NORMAL; //Return with exit state "NORMAL" (0)
}
