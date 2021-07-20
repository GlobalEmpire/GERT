#pragma once
#include "Socket.h"
#include <string>
#include <mutex>

#ifdef _WIN32
typedef unsigned long long SOCKET;
#else
typedef int SOCKET;
#endif

class DataPacket;

class Connection: public Socket {
    bool canWrite = true;
    bool outbound;
    bool ready = false;

    std::mutex safety;

protected:
    /**
     * Creates a new outbound connection to a given IP address.
     *
     * @param addr IPv6 address
     * @param port Remote port
     * @param name Name to use for connection in debug log
     */
    Connection(uint32_t addr, uint16_t port, const std::string& name);


    /**
     * Creates a new outbound connection to a given IP address.
     *
     * @param addr IPv6 address
     * @param port Remote port
     * @param name Name to use for connection in debug log
     */
    Connection(ipv6 addr, uint16_t port, const std::string& name);

    /**
     * Wraps an existing socket with an inbound connection.
     *
     * @param sock Socket being wrapped
     * @param name Name to use for connection in debug log
     */
    Connection(SOCKET sock, const std::string& name);

    virtual void _process() = 0;
    virtual bool handshake();

public:
    const std::string type;
    unsigned char vers[2] = {0, 0};

	void process() final;
	virtual void send(const DataPacket&) = 0;

	[[nodiscard]] std::string read(int=1);
	bool atleast(int);

	void write(const std::string&);

	bool valid() override;
};
