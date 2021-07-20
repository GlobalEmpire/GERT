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
#include "Util/Trace.h"
#include "Util/logging.h"
#include "Util/Error.h"
#include <exception> //Load exception library for terminate() hook
#include <iostream>
#include "Util/Versioning.h"
#include "Gateway/Key.h"

#ifdef WIN32
#include <windows.h>
#endif

using namespace std; //Default namespace to std so I don't have to type out std::cout or any other crap

enum status { //Create a list of error codes
	NORMAL,
	KEY_LOAD_ERR,
    UNKNOWN_MINOR,
	UNKNOWN_CRITICAL
};

volatile bool running = false; //SIGINT tracker
bool debugMode = false; //Set debug mode to false by default

uint16_t peerPort = 59474; //Set default peer port
uint16_t gatewayPort = 43780; //Set default gateway port

#ifdef WIN32
HANDLE sthread;

extern void apc(ULONG_PTR);
#endif

void shutdownProceedure([[maybe_unused]] int _) { //SIGINT handler function
	if (running) { //If we actually started running
		warn("User requested shutdown. Flipping the switch!"); //Notify the user because reasons
		running = false; //Flip tracker to disable threads and trigger main thread's cleanup

#ifdef _WIN32
        QueueUserAPC(apc, sthread, 0);
#endif
	} else { //We weren't actually running?
		error("Server wasn't in running state when SIGINT was raised."); //Warn user of potential error
		error("!!! Forcing termination of server. !!!"); //Warn user we are stopping
        cout << "\nAbnormal Exit\n";
		exit(UNKNOWN_MINOR); //Exit with correct exit code
	}
}

void OHCRAPOHCRAP([[maybe_unused]] int _) { //Uhm, we've caused a CPU error
	running = false;
	dumpStack();
	cout << "\nSIGSEGV THROWN: SEGMENTATION FAULT.\n";
	cout << "Please post a bug report including any potentially useful logs.\n";
    cout << "\nAbnormal Exit\n";
	exit(UNKNOWN_CRITICAL); //Exit with correct exit code
}

void errHandler() { //Error catcher, provides minor error recovery facilities
	running = false;
	cout << "Unknown error, system called terminate() with code " << to_string(errno) << "\n"; //Report fault to user
	cout << "Possibly " << std::make_error_code((std::errc)errno).message() << "\n";

	cout << "Testing for C++ exception.\n";

	const int exceptions = uncaught_exceptions();
	cout << "Uncaught exceptions: " << to_string(exceptions) << "\n";

	for (int i = 0; i < exceptions; i++) {
	    auto ptr = current_exception();

	    if (ptr) {
	        try {
                rethrow_exception(ptr);
	        } catch (std::exception& e) {
                cout << "Exception " << to_string(i) << ": " << e.what() << "\n";
	        }
	    }
	}

	if (exceptions == 0) {
        auto ptr = current_exception();

        if (ptr) {
            cout << "No exceptions but we found one?\n";

            try {
                rethrow_exception(ptr);
            } catch (std::exception& e) {
                cout << "Exception: " << e.what() << "\n";
            }
        }
	}


	cout << "\nAbnormal Exit\n";
	exit(UNKNOWN_CRITICAL); //Exit with correct exit code
}

void shandler([[maybe_unused]] int _) {}

inline void printHelp() { //Prints help on parameters
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
		} else if (curArg  == "-p") { //Process the local GEDS port flag
			if (argc == i) //If the port flag is missing the actual port
				printHelp(); //Print help, user might not know how to use the flag

            std::string nextArg = argv[++i];
			peerPort = std::stoul(nextArg); //Local GEDS port is set to the next argument. Also skips the next argument
		} else if (curArg == "-g") { //Process the gateway port flag
			if (argc == i) //If the port flag is missing the actual port
				printHelp(); //Print help, user might not know how to use the flag

            std::string nextArg = argv[++i];
			gatewayPort = std::stoul(nextArg); //Local GEDS port is set to the next argument. Also skips the next argument
		} else if (curArg == "-h") //Process the help flag
				printHelp(); //Prints help, user requested it
	}
}

int main( int argc, char* argv[] ) {
	cout << "GEDS Server v" << ThisVersion.stringify() << endl; //Print version information
	cout << "Copyright 2021" << endl; //Print simple copyright information

    startLog(); //Create log handles

	processArgs(argc, argv); //Process command line argument
	debug((string)"Processed arguments. Gateway port: " + to_string(gatewayPort) + " Peer port: " +
	    to_string(peerPort) + " and debug output"); //Print to debug processed arguments

#ifdef WIN32
    debug("Hello, this is an unreleased build. It has a few issues. Most notably, on some machines, decryption errors can cause crashing.");
    debug("If you encounter this bug, PLEASE contact me, I need the help. I have no idea what causes this. It should literally be impossible.");
#endif

	set_terminate(errHandler);

#ifdef _WIN32
	signal(SIGSEGV, &OHCRAPOHCRAP); //Catches the SIGSEGV CPU fault
	signal(SIGINT, &shutdownProceedure); //Hook SIGINT with custom handler
#else
	struct sigaction mem = { &OHCRAPOHCRAP };
	struct sigaction intr = { &shutdownProceedure };
	struct sigaction usr = { &shandler };

	sigaction(SIGSEGV, &mem, nullptr);
	sigaction(SIGINT, &intr, nullptr);
	sigaction(SIGUSR1, &usr, nullptr);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1); // Block SIGUSR1 to prevent racing
    pthread_sigmask(SIG_BLOCK, &mask, nullptr);
#endif

	debug("Loading key registrations");
	if (!Key::loadKeys("resolutions.geds"))
	    return KEY_LOAD_ERR;

	debug("Loading configuration");
	Config config{ "geds.conf" };

	Socket::init();

	debug("Building peer web"); //Use debug to notify user where we are in the loading process

#ifdef WIN32
    {
        auto proc = GetCurrentProcess();
        DuplicateHandle(proc, GetCurrentThread(), proc, &sthread, 0, false, DUPLICATE_SAME_ACCESS);
    }
#endif

	running = true; //We've officially started running! SIGINT is now not evil!

    debug("Starting main server loop"); //Use debug to notify user where we are in the loading process

    try {
        runServer(config); //Process incoming connections (not messages)
    } catch (std::exception& e) {
        error("Uncaught exception");
        throw e;
    }

    warn("Main server loop killed."); //Notify user we've stopped accepting incoming connections

    running = false;

#ifdef WIN32
    CloseHandle(sthread);
#endif

    Socket::deinit();

	stopLog(); //Release log handles

	return NORMAL; //Exit with correct exit code
}
