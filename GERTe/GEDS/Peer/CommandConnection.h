#pragma once
#include "../Networking/Connection.h"
#include "../Gateway/Address.h"
#include "CommandPacket.h"
#include <vector>

class CommandConnection: public Connection {
    template<class T>
    friend class Server;

    std::vector<Address> clients;
    std::vector<DataPacket> queued;
    CommandPacket* curPacket = nullptr;

    void _process() override;

    /**
     * Wraps an existing socket with an inbound connection.
     *
     * @param sock Socket being wrapped
     */
    explicit CommandConnection(SOCKET sock);

public:
    /**
     * Creates a new outbound connection to a given IP address.
     *
     * @param addr IPv4 address
     * @param port Remote port
     */
    CommandConnection(uint32_t addr, uint16_t port);

    /**
     * Creates a new outbound connection to a given IP address.
     *
     * @param addr IPv6 address
     * @param port Remote port
     */
    CommandConnection(ipv6, uint16_t);

    ~CommandConnection() noexcept override;

    void send(const DataPacket&) override;

    static void attempt(const DataPacket&);
};
