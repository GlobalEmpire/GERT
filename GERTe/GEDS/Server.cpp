#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <Ws2tcpip.h>
#else
#include <sys/socket.h> //Load C++ standard socket API
#include <netinet/tcp.h>
#include <unistd.h>
#endif

#include "Server.h"
#include "Connection.h"
#include "UGateway.h"
#include "Peer.h"
#include "logging.h"
#include "Poll.h"
#include "Processor.h"
#include "Error.h"

extern Poll netPoll;
extern Processor* proc;

Server::Server(unsigned short port, Server::Type type) : type{ type }, INet{ INet::Type::LISTEN } {
	sockaddr_in info = {
			AF_INET,
			htons(port),
			INADDR_ANY
	};

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	int res = ::bind(sock, (sockaddr*)&info, sizeof(sockaddr_in));

	if (res != 0) {
		if (errno == EADDRINUSE)
			error("Port " + std::to_string(port) + " is in use");
		else
			error(socketError("Error while starting the listen server: "), false);

		crash(ErrorCode::LIBRARY_ERROR);
	}
}

Server::~Server() {
#ifdef _WIN32
	closesocket(sock);
#else
	close(sock);
#endif
}

void Server::start() { 
	int res = listen(sock, SOMAXCONN);

	if (res != 0) {
		error(socketError("Error while starting the listen server: "), false);
		crash(ErrorCode::LIBRARY_ERROR);
	}
}

void Server::process() {
	SOCKET newSock = accept(sock, NULL, NULL);

	if (newSock == -1) {
		error(socketError("Error accepting a new connection: "), false);
		return;
	}

	//Winsock inheritance fix, undoes winsock event inhertiance

	Connection* newConn;

	try {
		if (type == Server::Type::GATEWAY) {
			newConn = new UGateway{ newSock };
		}
		else {
			newConn = new Peer{ newSock };
		}

		netPoll.add(newConn);

#ifdef _WIN32
		netPoll.remove(sock);
		netPoll.add(this);

		lock.unlock();

		proc->update();
#endif
	}
	catch ([[maybe_unused]]int e) {
#ifdef _WIN32
		closesocket(newSock);
#else
		close(newSock);
#endif

		error(socketError("Error accepting new connection: "), false);
	}
}
