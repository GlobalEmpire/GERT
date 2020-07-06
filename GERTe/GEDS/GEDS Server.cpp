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
typedef unsigned short ushort; //Created ushort shortcut for Unsigned Short

#include <signal.h> //Include signal processing API for error catching
#include "netty.h" //Include netcode header for entering server process
#include "overwatch.h" //Include overwatch header for error checking and "recovery"
#include "fileMngr.h" //Include file manager library for loading and saving databases
#include "Trace.h"
#include "logging.h"
#include <exception> //Load exception library for terminate() hook
#include <iostream>
#include "Versioning.h"
#include "Processor.h"
#include "Sentry.h"
using namespace std; //Default namespace to std so I don't have to type out std::cout or any other crap

enum status { //Create a list of error codes
	NORMAL, //YAY WE RAN AS WE SHOULD HAVE (0)
	PEER_LOAD_ERR, //Unknown error when loading peer database (3)
	KEY_LOAD_ERR, //Unknown error when loading key database (4)
	UNKNOWN_CRITICAL, //Unknown error which resulted in a crash, either terminate() or SIGSEGV (5)
	UNKNOWN_MINOR //Unknown error occured preceeding closing, not necessarily a crash (6)
};

volatile bool running = false; //SIGINT tracker
bool debugMode = false; //Set debug mode to false by default

char * LOCAL_IP = nullptr; //Set local address to predictable null value for testing
char * peerPort = "59474"; //Set default peer port
char * gatewayPort = "43780"; //Set default gateway port

extern Poll serverPoll;
extern Poll peerPoll;
extern Poll gatePoll;

void shutdownProceedure(int param) { //SIGINT handler function
	if (running) { //If we actually started running
		warn("User requested shutdown. Flipping the switch!"); //Notify the user because reasons
		running = false; //Flip tracker to disable threads and trigger main thread's cleanup

#ifdef _WIN32
		serverPoll.update(); //Work around Windows oddities
#endif
	} else { //We weren't actually running?
		error("Server wasn't in running state when SIGINT was raised."); //Warn user of potential error
		error("!!! Forcing termination of server. !!!"); //Warn user we are stopping
		Sentry::warn("SIGINT before RUNNING");
		Sentry::stop();
		exit(UNKNOWN_MINOR); //Exit with correct exit code
	}
};

void OHCRAPOHCRAP(int param) { //Uhm, we've caused a CPU error
	running = false;

	int errCode = emergencyScan(); //Scan structure database for faults, there will be faults
	saveResolutions(); //Save key resolutions, these don't easily if ever corrupt
	if (errCode < 2) { //If no peer error is detected (Wait what faulted?)
		savePeers(); //Save the peers database
		debug("Peers uncorrupted, peer file updated."); //Report to user using debug facility
	}
	dumpStack();
	Sentry::crash("SIGSEGV");
	cout << "Well, this is the end\n"; //Print a little poem to the user
	cout << "I've read too much, and written so far\n"; //Reference to memory error
	cout << "But here I am, faulting to death\n";
	cout << "\nSIGSEGV THROWN: SEGMENTATION FAULT.\n"; //Actual error
	cout << "SIGSEGV is a critical error thrown by the operating system\n"; //Information for uninformed user
	cout << "It signifies a MASSIVE error that won't correct itself and probably can't be fixed without changing the code.\n"; //Create sense of urgency in user
	cout << "Please post a bug report including any potentially useful logs.\n"; //Call to action trying to get user to submit report
	exit(UNKNOWN_CRITICAL); //Exit with correct exit code
}

void errHandler() { //Error catcher, provides minor error recovery facilities
	running = false;

	int errCode = emergencyScan(); //Scan structure databases for faults
	saveResolutions(); //Save key resolutions, these don't easily if ever corrupt
	if (errCode < 2) { //If no peer error is detected
		savePeers(); //Save the peers database
		debug("Peers uncorrupted, peer file updated."); //Report to user using debug facility
	}
	cout << "Unknown error, system called terminate() with code " << to_string(errno) << "\n"; //Report fault to user
	Sentry::error("Terminate " + to_string(errno));
	exit(UNKNOWN_CRITICAL); //Exit with correct exit code
}

inline void printHelp() { //Prints help on parameters
	cout << "Requires atleast the -a parameter\n";
	cout << "-a publicIP   Specifies the public IP to prevent loops.\n";
	cout << "-p port       Specifies the port for GEDS servers to connect to (default 59474)\n";
	cout << "-g port       Specifies the port for Gateways to connect to (default 43780)\n";
	cout << "-h            Displays this message\n";
	cout << "-d            Prints out extra debug information\n";
	cout << "-r            Automatically report crashes to Sentry\n";
	exit(0);
}

void processArgs(int argc, char* argv[]) { //Process and interpret command line arguments
	for (int i = 1; i < argc; i++) { //Loop through the arguments
		string curArg = argv[i]; //Convert it to a string for easier access
		if (curArg == "-d") { //Process the -d Debug messages flag
			debugMode = true;
		}
		else if (curArg == "-a") { //Process the local address flag (REQUIRED)
			if (argc == i) //Address flag is missing the actual address
				printHelp(); //Print help, user might not know how to use the flag
			LOCAL_IP = argv[++i]; //Local address set to next argument. Also skips the next argument
		}
		else if (curArg == "-p") { //Process the local GEDS port flag
			if (argc == i) //If the port flag is missing the actual port
				printHelp(); //Print help, user might not know how to use the flag
			peerPort = argv[++i]; //Local GEDS port is set to the next argument. Also skips the next argument
		}
		else if (curArg == "-g") { //Process the gateway port flag
			if (argc == i) //If the port flag is missing the actual port
				printHelp(); //Print help, user might not know how to use the flag
			gatewayPort = argv[++i]; //Local GEDS port is set to the next argument. Also skips the next argument
		}
		else if (curArg == "-h") //Process the help flag
			printHelp(); //Prints help, user requested it
		else if (curArg == "-r")
			online_reporter = true;
	}
	if (LOCAL_IP == nullptr) //If the local address was not set
		printHelp(); //User forgot to include local address, print help in case they don't know they have to
}

int main( int argc, char* argv[] ) {

	cout << "GEDS Server v" << ThisVersion.stringify() << endl; //Print version information
	cout << "Copyright 2017" << endl; //Print simple copyright information

	processArgs(argc, argv); //Process command line argument
	debug((string)"Processed arguments. Gateway port: " + gatewayPort + " Peer port: " + peerPort + " Local IP: " + LOCAL_IP + " and debug output"); //Print to debug processed arguments

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

	Sentry::init();

	debug("Loading peers"); //Use debug to notify user where we are in the loading process
	int result = loadPeers(); //Load the peer database

	if (result == -1)
		return PEER_LOAD_ERR;

	debug("Loading resolutions"); //Use debug to notify user where we are in the loading process
	result = loadResolutions(); //Load the key resolution database

	if (result == -1)
		return KEY_LOAD_ERR;

	debug("Starting servers"); //Use debug to notify user where we are in the loading process
	startup(); //Startup server sockets

	debug("Building peer web"); //Use debug to notify user where we are in the loading process
	buildWeb(); //Connect to online peers and update connection database

	running = true; //We've officially started running! SIGINT is now not evil!

	debug("Starting gateway message processor"); //Use debug to notify user where we are in the loading process
	Processor * gateways = new Processor{ &gatePoll };

	debug("Starting peer message processor");
	Processor * peers = new Processor{ &peerPoll };

	debug("Starting main server loop"); //Use debug to notify user where we are in the loading process
	runServer(); //Process incoming connections (not messages)
	warn("Primary server killed."); //Notify user we've stopped accepting incoming connections

	//Shutdown and Cleanup sequence
	debug("Cleaning up servers"); //Notify user where we are in the shutdown process
	cleanup(); //Cleanup servers

	debug("Waiting for message processors to exit"); //Notify user where we are in the shutdown process
	delete gateways;
	debug("Gateway processor exited");
	delete peers;

	warn("Processors killed, program ending."); //Notify the user we've stopped processing messages
	
	debug("Saving peers"); //Notify user where we are in the shutdown process
	savePeers(); //Save the peers database to file

	debug("Saving resolutions"); //Notify user where we are in the shutdown process
	saveResolutions(); //Save the keys database to file

	Sentry::stop();

	stopLog(); //Release log handles

	return NORMAL; //Exit with correct exit code
}
