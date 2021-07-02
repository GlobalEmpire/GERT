#ifndef _WIN32
#include <sys/socket.h> //Load C++ standard socket API
#include <netinet/tcp.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <netinet/ip.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#endif
#include <thread>
#include "../Peer/query.h"
#include "../Util/logging.h"
#include "../Threading/Poll.h"
#include "../Util/Error.h"
#include "../Threading/Processor.h"
#include "../Gateway/DataConnection.h"
#include "../Peer/CommandConnection.h"
#include "../Util/Error.h"

using namespace std;

SOCKET gateServer, gedsServer; //Define both server sockets

Poll socketPoll;
Poll serverPoll;

#ifdef _WIN32
bool wsastarted = false;
#endif

extern volatile bool running;
extern unsigned short gatewayPort;
extern unsigned short peerPort;
extern char * LOCAL_IP;

SOCKET createSocket(uint32_t ip, unsigned short port) {
#ifdef _WIN32 //If compiled for Windows
    if (!wsastarted) {
        WSADATA socketConfig; //Construct WSA configuration destination
        WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock
    }
#endif

    sockaddr_in tgt = {
            AF_INET,
            htons(gatewayPort),
            INADDR_ANY
    };

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //Bind servers to addresses
    if (connect(sock, (sockaddr*)&tgt, sizeof(sockaddr_in)) != 0) {
        socketError("Failed connecting to peer: ");
        return -1;
    }

    return sock;
}

void startup() {
	//Server construction
#ifdef _WIN32 //If compiled for Windows
    if (!wsastarted) {
        WSADATA socketConfig; //Construct WSA configuration destination
        WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock
    }
#endif

	sockaddr_in gate = {
			AF_INET,
			htons(gatewayPort),
			INADDR_ANY
	};
	sockaddr_in geds = {
			AF_INET,
			htons(peerPort),
			INADDR_ANY
	};

	//Construct server sockets
	gateServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Construct gateway inbound socket
	gedsServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //Construct GEDS P2P inbound socket

	//Bind servers to addresses
	if (::bind(gateServer, (sockaddr*)&gate, sizeof(sockaddr_in)) != 0 && errno == EADDRINUSE) { //Initialize gateway inbound socket
		error("Gateway port is in use");
		exit(-1);
	}
	if (::bind(gedsServer, (sockaddr*)&geds, sizeof(sockaddr_in)) != 0 && errno == EADDRINUSE) { //Initialize GEDS P2P inbound socket
		error("Peer port is in use");
		exit(-1);
	}

	//Activate servers
	listen(gateServer, SOMAXCONN); //Open gateway inbound socket
	listen(gedsServer, SOMAXCONN); //Open gateway inbound socket
	//Servers constructed and started

	//Add servers to poll
	serverPoll.add(gateServer);
	serverPoll.add(gedsServer);
}

void cleanup() {
#ifdef _WIN32
	closesocket(gedsServer);
	closesocket(gateServer);

	WSACleanup();
#else
	close(gedsServer);
	close(gateServer);
#endif

	//Maybe later cleanup DataConnections and CommandConnections.
}

void runServer(std::vector<CommandConnection*> conns) { //Listen for new connections
    for (auto conn: conns)
        socketPoll.add(conn->sock, conn);
    socketPoll.update();

    std::thread serverThread { []() -> void {
            while (running) { //Dies on SIGINT
                Event_Data data = serverPoll.wait();

                if (data.fd == 0) {
                    return;
                }

                SOCKET newSock = accept(data.fd, nullptr, nullptr);

                if (newSock == -1) {
                    socketError("Error accepting a new connection: ");
                    continue;
                }

#ifdef _WIN32
                serverPoll.remove(data.fd);
                serverPoll.add(data.fd);
#endif

                try {
                    if (data.fd == gateServer) {
                        auto * gate = new DataConnection(newSock);
                        socketPoll.add(newSock, gate);

#ifdef _WIN32
                        socketPoll.update();
#endif
                    }
                    else {
                        auto peer = new CommandConnection(newSock);
                        socketPoll.add(newSock, peer);

#ifdef _WIN32
                        socketPoll.update();
#endif
                    }
                }
                catch ([[maybe_unused]] int e) {
#ifdef _WIN32
                    closesocket(newSock);
#else
                    close(newSock);
#endif
                }
            }
        }
    };

    serverPoll.claim(&serverThread, 1);
    serverThread.join();
}
