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

extern Poll clientPoll;
extern Processor* proc;

Server::Server(unsigned short port, Server::Type type) : type{ type }, INet{ INet::Type::LISTEN } {
	sockaddr_in info = {
			AF_INET,
			htons(port),
			INADDR_ANY
	};

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (::bind(sock, (sockaddr*)& info, sizeof(sockaddr_in)) != 0 && errno == EADDRINUSE) {
		error("Port " + std::to_string(port) + " is in use");
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
	listen(sock, SOMAXCONN);
}

void Server::process() {
	SOCKET newSock = accept(sock, NULL, NULL);

	if (newSock == -1) {
		socketError("Error accepting a new connection: ");
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

		clientPoll.add(newSock, newConn);

#ifdef _WIN32
		proc->update();
#endif
	}
	catch (int e) {
#ifdef _WIN32
		closesocket(newSock);
#else
		close(newSock);
#endif

		socketError("Error accepting new connection: ");
	}
}
