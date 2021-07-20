#ifndef WIN32
#include <csignal>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "../Threading/Poll.h"
#include "../Threading/Processor.h"
#include "../Gateway/DataConnection.h"
#include "Server.h"
#include "../Util/logging.h"

using namespace std;

extern uint16_t gatewayPort;
extern uint16_t peerPort;

void runServer(const Config& config) { //Listen for new connections
    Poll socketPoll;
    Processor processor{ &socketPoll };

    for (auto& peer: config.peers4)
        try {
            processor.add(new CommandConnection{peer.first, peer.second});
        } catch(std::exception& e) {
            error("Failed to connect to " + Config::stringify(peer.first) + ": " + e.what());
        }

    for (auto& peer: config.peers6)
        try {
            processor.add(new CommandConnection{peer.first, peer.second});
        } catch(std::exception& e) {
            error("Failed to connect to " + Config::stringify(peer.first) + ": " + e.what());
        }

#ifdef WIN32
    // Alternative: Enable Dual-Stack mode. Too lazy.
    processor.add(new Server<DataConnection>{false, gatewayPort, &processor});
    processor.add(new Server<CommandConnection>{ false, peerPort, &processor });
#endif

    processor.add(new Server<DataConnection>{true, gatewayPort, &processor});
    processor.add(new Server<CommandConnection>{ true, peerPort, &processor });

#ifdef WIN32
    SleepEx(INFINITE, true);
#else
    // Linux doesn't really have thread suspension, but we don't really need it.
    sigset_t mask;
    pthread_sigmask(0, nullptr, &mask);
    sigdelset(&mask, SIGUSR1); // Unblock SIGUSR1

    sigsuspend(&mask);
#endif
}
