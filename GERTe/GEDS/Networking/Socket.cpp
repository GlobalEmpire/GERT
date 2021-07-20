#include "Socket.h"
#include "../Util/logging.h"

#ifdef WIN32
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#define INVALID_SOCKET -1
#include <unistd.h>
#include <cerrno>
#include <arpa/inet.h>
#endif

#ifdef WIN32
#include <mutex>

std::mutex wsaMutex;

std::atomic<bool> Socket::wsaInit = false;
#endif

void Socket::init() {
#ifdef WIN32
    debug("Starting WinSock");

    std::lock_guard guard{ wsaMutex };

    if (!wsaInit) {
        WSADATA socketConfig; //Construct WSA configuration destination
        WSAStartup(MAKEWORD(2, 2), &socketConfig); //Initialize Winsock

        wsaInit = true;
    }
#endif
}

void Socket::deinit() {
#ifdef WIN32
    debug("Stopping WinSock");

    std::lock_guard guard{ wsaMutex };

    if (wsaInit) {
        WSACleanup();

        wsaInit = false;
    }
#endif
}

bool Socket::wouldBlock() {
#ifdef WIN32
        return WSAGetLastError() == WSAEWOULDBLOCK;
#else
        return errno == EWOULDBLOCK || errno == EAGAIN;
#endif
}

Socket::Socket(bool ip6, bool stream) {
#ifdef WIN32
    if (!wsaInit)
        throw std::runtime_error{ "Socket layer not initalized" };
#endif

    const int fam = ip6 ? AF_INET6 : AF_INET;
    const int type = stream ? SOCK_STREAM : SOCK_DGRAM;
    const int proto = stream ? IPPROTO_TCP : IPPROTO_UDP;

    sock = socket(fam, type, proto);
}

Socket::Socket(SOCKET sock): sock(sock) {
#ifdef WIN32
    if (!wsaInit)
        throw std::runtime_error{ "Socket layer not initalized" };
#endif
}

Socket::~Socket() {
    if (sock != INVALID_SOCKET)
#ifdef WIN32
        closesocket(sock);
#else
        ::close(sock);
#endif
}

void Socket::close() {
    if (sock == INVALID_SOCKET)
        return;

#ifdef WIN32
    closesocket(sock);
#else
    ::close(sock);
#endif

    sock = INVALID_SOCKET;
}

bool Socket::valid() {
    return sock != INVALID_SOCKET;
}
