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

using namespace std;

SOCKET gateServer, gedsServer; //Define both server sockets

Poll socketPoll;
Poll serverPoll;

extern volatile bool running;
extern unsigned short gatewayPort;
extern unsigned short peerPort;
extern char * LOCAL_IP;

constexpr unsigned int iplen = sizeof(sockaddr);

void killConnections() {
    // TODO: FIX
	/*for (gatewayIter iter; !iter.isEnd(); iter++) {
		(*iter)->close();
		delete *iter;
	}
	for (peerIter iter; !iter.isEnd(); iter++) {
		(*iter)->close();
	}
	for (noAddrIter iter; !iter.isEnd(); iter++) {
		(*iter)->close();
		delete *iter;
	}*/
}

void startup() {
	//Server construction
#ifdef _WIN32 //If compiled for Windows
	WSADATA socketConfig; //Construct WSA configuration destination
	WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock
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
#else
	close(gedsServer);
	close(gateServer);
#endif

	killConnections();
}

void runServer() { //Listen for new connections
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

void buildWeb() {
    // TODO: Redo
	/*for (auto & iter : peerList) {
		IP ip = iter.first;
		Ports ports = iter.second;

		if (ip.stringify() == LOCAL_IP)
			continue;
		else if (ports.peer == 0) {
			debug("Skipping peer " + ip.stringify() + " because its outbound only.");
			continue;
		}

		SOCKET newSock = socket(AF_INET, SOCK_STREAM, 0);
		in_addr remoteIP = ip.addr;
		sockaddr_in addrFormat;
		addrFormat.sin_addr = remoteIP;
		addrFormat.sin_port = ports.peer;
		addrFormat.sin_family = AF_INET;

#ifndef _WIN32
		int opt = 3;
		setsockopt(newSock, IPPROTO_TCP, TCP_SYNCNT, (void*)&opt, sizeof(opt)); //Correct excessive timeout period on Linux
#endif

		int result = connect(newSock, (sockaddr*)&addrFormat, iplen);

		if (result != 0) {
			warn("Failed to connect to " + ip.stringify() + " " + to_string(errno));
			continue;
		}

		Peer * newConn = new Peer(newSock, ip);

		newConn->write(ThisVersion.tostring());

		char death[3];
		timeval timeout = { 2, 0 };

		setsockopt(newSock, IPPROTO_TCP, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeval));
		int result2 = recv(newSock, death, 2, 0);

		if (result2 == -1) {
			delete newConn;
			error("Connection to " + ip.stringify() + " dropped during negotiation");
			continue;
		}

		if (death[0] == 0) {
			recv(newSock, death + 2, 1, 0);

			if (death[2] == 0) {
				warn("Peer " + ip.stringify() + " doesn't support " + ThisVersion.stringify());
				delete newConn;
				continue;
			}
			else if (death[2] == 1) {
				error("Peer " + ip.stringify() + " rejected this IP!");
				delete newConn;
				continue;
			}
		}

		log("Connected to " + ip.stringify());

		socketPoll.add(newSock, newConn);
	}*/
}