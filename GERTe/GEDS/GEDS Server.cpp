/* 
	This is the primary code for the GEDS server.
	This program implements platform independent definitions for internet sockets.
	Supported platforms are currently Linux(/Unix), Windows
	
	This code is designed to function with other code from
	https://github.com/GlobalEmpire/GERT/
	
	This code falls under the license located at
	https://github.com/GlobalEmpire/GERT/blob/master/License.md
 */
#include <csignal> //Include signal processing API for error catching
#include "Networking/netty.h" //Include netcode header for entering server process
#include "Files/fileMngr.h" //Include file manager library for loading and saving databases
#include "Util/Trace.h"
#include "Util/logging.h"
#include <exception> //Load exception library for terminate() hook
#include <iostream>
#include "Util/Versioning.h"
#include "Threading/Processor.h"
using namespace std; //Default namespace to std so I don't have to type out std::cout or any other crap

enum status { //Create a list of error codes
	NORMAL,
	KEY_LOAD_ERR,
	UNKNOWN_CRITICAL,
	UNKNOWN_MINOR
};

volatile bool running = false; //SIGINT tracker
bool debugMode = false; //Set debug mode to false by default

char * LOCAL_IP = nullptr; //Set local address to predictable null value for testing
unsigned short peerPort = 59474; //Set default peer port
unsigned short gatewayPort = 43780; //Set default gateway port

extern Poll serverPoll;
extern Poll socketPoll;

void shutdownProceedure([[maybe_unused]] int param) { //SIGINT handler function
	if (running) { //If we actually started running
		warn("User requested shutdown. Flipping the switch!"); //Notify the user because reasons
		running = false; //Flip tracker to disable threads and trigger main thread's cleanup

#ifdef _WIN32
		serverPoll.update(); //Work around Windows oddities
#endif
	} else { //We weren't actually running?
		error("Server wasn't in running state when SIGINT was raised."); //Warn user of potential error
		error("!!! Forcing termination of server. !!!"); //Warn user we are stopping
		exit(UNKNOWN_MINOR); //Exit with correct exit code
	}
}

void OHCRAPOHCRAP([[maybe_unused]] int param) { //Uhm, we've caused a CPU error
	running = false;
	dumpStack();
	cout << "\nSIGSEGV THROWN: SEGMENTATION FAULT.\n";
	cout << "Please post a bug report including any potentially useful logs.\n";
	exit(UNKNOWN_CRITICAL); //Exit with correct exit code
}

void errHandler() { //Error catcher, provides minor error recovery facilities
	running = false;
	cout << "Unknown error, system called terminate() with code " << to_string(errno) << "\n"; //Report fault to user
	exit(UNKNOWN_CRITICAL); //Exit with correct exit code
}

inline void printHelp() { //Prints help on parameters
	cout << "Requires atleast the -a parameter\n";
	cout << "-a publicIP   Specifies the public IP to prevent loops.\n";
	cout << "-p port       Specifies the port for GEDS servers to connect to (default 59474)\n";
	cout << "-g port       Specifies the port for Gateways to connect to (default 43780)\n";
	cout << "-h            Displays this message\n";
	cout << "-d            Prints out extra debug information\n";
	exit(0);
}

void processArgs(int argc, char* argv[]) { //Process and interpret command line arguments
	for (int i = 1; i < argc; i++) { //Loop through the arguments
		string curArg = argv[i]; //Convert it to a string for easier access
		if (curArg == "-d") { //Process the -d Debug messages flag
			debugMode = true;
		} else if (curArg == "-a") { //Process the local address flag (REQUIRED)
			if (argc == i) //Address flag is missing the actual address
				printHelp(); //Print help, user might not know how to use the flag
			LOCAL_IP = argv[++i]; //Local address set to next argument. Also skips the next argument
		} else if (curArg  == "-p") { //Process the local GEDS port flag
			if (argc == i) //If the port flag is missing the actual port
				printHelp(); //Print help, user might not know how to use the flag
			peerPort = atoi(argv[++i])  ; //Local GEDS port is set to the next argument. Also skips the next argument
		} else if (curArg == "-g") { //Process the gateway port flag
			if (argc == i) //If the port flag is missing the actual port
				printHelp(); //Print help, user might not know how to use the flag
			gatewayPort = atoi(argv[++i]); //Local GEDS port is set to the next argument. Also skips the next argument
		} else if (curArg == "-h") //Process the help flag
				printHelp(); //Prints help, user requested it
	}
	if (LOCAL_IP == nullptr) //If the local address was not set
		printHelp(); //User forgot to include local address, print help in case they don't know they have to
}

int main( int argc, char* argv[] ) {
	cout << "GEDS Server v" << ThisVersion.stringify() << endl; //Print version information
	cout << "Copyright 2017" << endl; //Print simple copyright information

	processArgs(argc, argv); //Process command line argument
	debug((string)"Processed arguments. Gateway port: " + to_string(gatewayPort) + " Peer port: " + to_string(peerPort) + " Local IP: " + LOCAL_IP + " and debug output"); //Print to debug processed arguments

	startLog(); //Create log handles

	set_terminate(errHandler);

#ifdef _WIN32
	signal(SIGSEGV, &OHCRAPOHCRAP); //Catches the SIGSEGV CPU fault
	signal(SIGINT, &shutdownProceedure); //Hook SIGINT with custom handler
#else
	struct sigaction mem = { 0 };
	struct sigaction intr = { 0 };

	mem.sa_handler = OHCRAPOHCRAP;
	intr.sa_handler = shutdownProceedure;

	sigaction(SIGSEGV, &mem, nullptr);
	sigaction(SIGINT, &intr, nullptr);
#endif
	debug("Loading key registrations");
	if (!loadResolutions())
	    return KEY_LOAD_ERR;

	debug("Starting servers"); //Use debug to notify user where we are in the loading process
	startup(); //Startup server sockets

	debug("Building peer web"); //Use debug to notify user where we are in the loading process
	auto peers = loadPeers();

	running = true; //We've officially started running! SIGINT is now not evil!

	debug("Starting transport processor"); //Use debug to notify user where we are in the loading process
    { // New block to abuse built-in C++ memory management.
        Processor gateways{ &socketPoll };

        debug("Starting main server loop"); //Use debug to notify user where we are in the loading process
        runServer(peers); //Process incoming connections (not messages)
        warn("Primary server killed."); //Notify user we've stopped accepting incoming connections

        //Shutdown and Cleanup sequence
        debug("Cleaning up servers"); //Notify user where we are in the shutdown process
        cleanup(); //Cleanup servers
        debug("Waiting for transport processor to exit"); //Notify user where we are in the shutdown process
    }
	debug("Transport processor exited");

	stopLog(); //Release log handles

	return NORMAL; //Exit with correct exit code
}
