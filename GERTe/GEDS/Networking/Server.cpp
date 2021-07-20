#include "../Util/Error.h"
#include <stdexcept>

#ifndef _WIN32
#include <sys/socket.h> //Load C++ standard socket API
#include <arpa/inet.h>
#else
#include <WinSock2.h>
#include <ws2ipdef.h>

#pragma comment(lib, "Ws2_32.lib")
#endif

template<class T>
Server<T>::Server(bool ip6, uint16_t port, Processor* processor): Socket(ip6, true), processor(processor) {
    if (ip6) {
        sockaddr_in6 addr = {
                AF_INET6,
                htons(port),
                0,
                in6addr_any,
                0
        };

        if (::bind(sock, (sockaddr *)&addr, sizeof(sockaddr_in6)) != 0)
            throw std::runtime_error{socketError("Server bind failed: ") };
    } else {
        sockaddr_in addr = {
                AF_INET,
                htons(port),
                INADDR_ANY
        };

        if (::bind(sock, (sockaddr *)&addr, sizeof(sockaddr_in)) != 0)
            throw std::runtime_error{ socketError("Server bind failed: ") };
    }

    listen(sock, SOMAXCONN);
}

template<class T>
void Server<T>::process() {
    SOCKET nsock;
    while ((nsock = accept(sock, nullptr, nullptr)) != -1)
        processor->add(new T(nsock));

    if (!wouldBlock())
        throw std::runtime_error{ socketError("Failed to accept socket: ") };

#ifdef WIN32
    // Fix WIN32 buggy listen events
    processor->remove(this);
    processor->add(this);
#endif
}
