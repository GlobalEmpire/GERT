#pragma once
#include "../Threading/Processor.h"
#include "Connection.h"
#include <type_traits>

template<class T>
class Server: public Socket {
    static_assert(std::is_base_of<Connection, T>::value, "T must derive from Connection");

    Processor* processor;

public:
    /**
     * Creates and binds a new server.
     * <br>
     * <br>
     * Note: Due to a bug(?) on Windows, the server must also be part of processor.
     *
     * @param ip6 If this server should use IPv6
     * @param port Which port to bind to
     * @param processor Processor to pass new connections to
     */
    Server(bool ip6, uint16_t port, Processor* processor);

    void process() final;
};

#include "Server.cpp"
