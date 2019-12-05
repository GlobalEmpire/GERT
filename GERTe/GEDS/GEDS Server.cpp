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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <signal.h> //Include signal processing API for error catching
#include "netty.h" //Include netcode header for entering server process
#include "fileMngr.h" //Include file manager library for loading and saving databases
#include "Trace.h"
#include "logging.h"
#include <exception> //Load exception library for terminate() hook
#include <iostream>
#include "Versioning.h"
#include "Processor.h"
#include "Error.h"
using namespace std; //Default namespace to std so I don't have to type out std::cout or any other crap

volatile bool running = false; //SIGINT tracker
bool debugMode = false; //Set debug mode to false by default

char * LOCAL_IP = nullptr; //Set local address to predictable null value for testing
unsigned short peerPort = 59474;
unsigned short gatewayPort = 43780;

#ifdef _WIN32
HANDLE thisThread = INVALID_HANDLE_VALUE;
#endif

void shutdownProceedure([[maybe_unused]] int param) { //SIGINT handler function
	if (running) { //If we actually started running
		warn("User requested shutdown. Flipping the switch!");	//Notify the user because reasons
		running = false; //Flip tracker to disable threads and trigger main thread's cleanup

#ifdef _WIN32
		ResumeThread(thisThread);								// Windows Workaround
#endif
	} else { //We weren't actually running?
		error("Server wasn't in running state when SIGINT was raised."); //Warn user of potential error
		error("!!! Forcing termination of server. !!!"); //Warn user we are stopping
		stopLog();
		crash(ErrorCode::GENERAL_ERROR);
	}
};

void OHCRAPOHCRAP([[maybe_unused]] int param) { //Uhm, we've caused a CPU error
	running = false;

	saveResolutions(); //Save key resolutions, these don't easily if ever corrupt
	savePeers();

	dumpStack();
	error("SEGMENTATION FAULT");
	stopLog();
	crash(ErrorCode::GENERAL_ERROR);
}

void errHandler() { //Error catcher, provides minor error recovery facilities
	running = false;

	saveResolutions(); //Save key resolutions, these don't easily if ever corrupt
	savePeers();

	error("Unknown error, system called terminate() with code " + to_string(errno) + "\n");
	stopLog();
	crash(ErrorCode::GENERAL_ERROR);
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
			peerPort = (unsigned short)std::stoi(argv[++i]); //Local GEDS port is set to the next argument. Also skips the next argument
		} else if (curArg == "-g") { //Process the gateway port flag
			if (argc == i) //If the port flag is missing the actual port
				printHelp(); //Print help, user might not know how to use the flag
			gatewayPort = (unsigned short)std::stoi(argv[++i]); //Local GEDS port is set to the next argument. Also skips the next argument
		} else if (curArg == "-h") //Process the help flag
				printHelp(); //Prints help, user requested it
	}
	if (LOCAL_IP == nullptr) //If the local address was not set
		printHelp(); //User forgot to include local address, print help in case they don't know they have to
}

int main( int argc, char* argv[] ) {

	cout << "GEDS Server v" << ThisVersion.stringify() << endl; //Print version information
	cout << "Written by Tyler Kuhn (TYKUHN2)" << endl;			//Print author information
	cout << "Copyright 2017-2019" << endl;						//Print simple copyright information

	processArgs(argc, argv); //Process command line argument
	debug((string)"Processed arguments. Gateway port: " + std::to_string(gatewayPort) + " Peer port: " + std::to_string(peerPort) + " Local IP: " + LOCAL_IP + " and debug output"); //Print to debug processed arguments

	startLog(); //Create log handles

	set_terminate(errHandler);

#ifdef _WIN32
	signal(SIGSEGV, &OHCRAPOHCRAP); //Catches the SIGSEGV CPU fault

	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &thisThread, NULL, FALSE, DUPLICATE_SAME_ACCESS);

	signal(SIGINT, &shutdownProceedure); //Hook SIGINT with custom handler
#else
	struct sigaction mem = { 0 };
	struct sigaction intr = { 0 };

	mem.sa_handler = OHCRAPOHCRAP;
	intr.sa_handler = shutdownProceedure;

	sigaction(SIGSEGV, &mem, nullptr);
	sigaction(SIGINT, &intr, nullptr);
#endif

	debug("Loading peers"); //Use debug to notify user where we are in the loading process
	int result = loadPeers(); //Load the peer database

	if (result == -1)
		crash(ErrorCode::PEER_LOAD_ERROR);

	debug("Loading resolutions"); //Use debug to notify user where we are in the loading process
	result = loadResolutions(); //Load the key resolution database

	if (result == -1)
		crash(ErrorCode::KEY_LOAD_ERROR);

	debug("Starting servers"); //Use debug to notify user where we are in the loading process
	startup(gatewayPort, peerPort); //Startup server sockets

	debug("Building peer web"); //Use debug to notify user where we are in the loading process
	buildWeb(LOCAL_IP); //Connect to online peers and update connection database

	running = true; //We've officially started running! SIGINT is now not evil!

	debug("Starting server"); //Use debug to notify user where we are in the loading process
	runServer(); //Process incoming connections (not messages)
	warn("Server killed."); //Notify user we've stopped accepting incoming connections

	//Shutdown and Cleanup sequence
	debug("Cleaning up servers"); //Notify user where we are in the shutdown process
	cleanup(); //Cleanup servers
	
	debug("Saving peers"); //Notify user where we are in the shutdown process
	savePeers(); //Save the peers database to file

	debug("Saving resolutions"); //Notify user where we are in the shutdown process
	saveResolutions(); //Save the keys database to file

	stopLog(); //Release log handles

	return 0;						//Exit successfully
}
