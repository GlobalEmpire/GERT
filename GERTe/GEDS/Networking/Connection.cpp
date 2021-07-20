#ifdef _WIN32
#define ioctl ioctlsocket

#include <WinSock2.h>
#include <ws2ipdef.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#define INVALID_SOCKET -1

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

typedef unsigned long u_long;
#endif

#include "Connection.h"
#include "../Util/logging.h"
#include "../Util/Versioning.h"
#include "../Util/Error.h"
#include <stdexcept>
#include <cstring>

static_assert(sizeof(ipv6) == sizeof(in6_addr), "ipv6 != in6_addr length");

constexpr unsigned long nonblock = 1;

bool Connection::handshake() {
    return true;
}

Connection::Connection(uint32_t addr, uint16_t port, const std::string& name) : Socket(false, true),
                                                                                type(name), outbound(true)  {
    sockaddr_in tgt = {
            AF_INET,
            htons(port)
    };

    memcpy(&tgt.sin_addr, &addr, sizeof(addr));

    if (connect(sock, (sockaddr*)&tgt, sizeof(sockaddr_in)))
        throw std::runtime_error{ socketError("Failed to connect: ") };

    ioctl(sock, FIONBIO, (u_long*)&nonblock); //Ensure socket is in blocking mode

    write(ThisVersion.tostring()); // We write first
}

Connection::Connection(ipv6 addr, uint16_t port, const std::string& name) : Socket(true, true),
                                                                            type(name), outbound(true)  {
    sockaddr_in6 tgt = {
            AF_INET6,
            htons(port),
            0,
            {},
            0
    };

    memcpy(&tgt.sin6_addr, &addr, sizeof(addr));

    if (connect(sock, (sockaddr*)&tgt, sizeof(sockaddr_in6)))
        throw std::runtime_error{socketError("Failed to connect: ") };

    ioctl(sock, FIONBIO, (u_long*)&nonblock); //Ensure socket is in blocking mode

    write(ThisVersion.tostring()); // We write first
}

Connection::Connection(SOCKET socket, const std::string& type) : Socket(socket), type(type), outbound(false) {
#ifdef WIN32
	WSAEventSelect(socket, nullptr, 0); //Clears all events associated with the new socket
#endif

	ioctl(sock, FIONBIO, (u_long*)&nonblock); //Ensure socket is in blocking mode
}

void Connection::process() {
    if (vers[0] == 0 && vers[1] == 0) { // Pre-handshake
        size_t result = ::recv(sock, (char*)vers, 2, MSG_PEEK);

        if (result <= 0) {
            error(socketError("Error reading version information: "));
            close();
            return;
        } else if (result == 1) // Not enough bytes yet
            return;

        ::recv(sock, (char*)vers, 2, 0);

        if (vers[0] == 0 && vers[1] == 0) {
            char errcode;
            result = ::recv(sock, &errcode, 1, MSG_WAITALL);
            close();

            if (result < 1)
                error(socketError("Error processing handshake error: "));
            else
                switch (errcode) {
                    case 0:
                        error(type + " reported a version error.");
                        break;
                    case 1:
                        error(type + " reported a unknown address error. This is abnormal.");
                        break;
                    case 2:
                        error(type + " reported an unknown error.");
                        break;
                    default:
                        error(type + " reported an unrecognized error.");
                }

            return;
        }


        if (vers[0] != ThisVersion.major) { //Determine if major number is not supported
            write(std::string{"\0\0\0", 3});
            warn(type + "'s version wasn't supported!");
            close();
            return;
        }

        log(type + " using v" + std::to_string(vers[0]) + "." + std::to_string(vers[1]));

        if (vers[1] > ThisVersion.minor)
            vers[1] = ThisVersion.minor;
    }

    if (!ready) {
        if (!handshake())
            return;

        ready = true;
        if (!outbound)
            write(ThisVersion.tostring()); // We still need to write
    }

    std::lock_guard guard{ safety };
    while (atleast(1))
        _process();
}

std::string Connection::read(int num) {
    if (num == 0) {
        warn("Connection tried to read 0, this is usually a bug.");
        return "";
    }

    if (sock == INVALID_SOCKET)
        return "";

    char * buf = new char[num];
    size_t len = recv(sock, buf, num, 0);
    if (len == -1) {
        if (wouldBlock())
            return "";

        close();

        throw std::runtime_error{ socketError("Failed to read from socket") };
    } else if (len == 0) {
        close(); // We ought to get something, so close for safety.
        return "";
    }

    std::string data{ buf, (size_t)len };
    delete[] buf;
    return data;
}

bool Connection::atleast(int len) {
    char* dump = new char[len];
    size_t num = recv(sock, dump, len, MSG_PEEK);
    delete[] dump;

    if (num == -1 && !wouldBlock())
        close();

    return num == len;
}

void Connection::write(const std::string& data) {
    if (!canWrite || sock == INVALID_SOCKET)
        return;

    if (::send(sock, data.c_str(), data.length(), 0) == -1) {
        canWrite = false;

        while (valid())
            process();

        close();
    }
}

bool Connection::valid() {
    if (!Socket::valid())
        return false;

    char discard;
    bool valid = recv(sock, &discard, 1, MSG_PEEK) > 0 || wouldBlock();

    if (!valid)
        close();

    return valid;
}
