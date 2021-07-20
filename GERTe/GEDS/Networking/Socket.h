#pragma once
#include <cstdint>

#ifdef _WIN32
#include <atomic>

typedef unsigned long long SOCKET;
#else
typedef int SOCKET;
#endif

struct ipv6 {
    uint64_t low;
    uint64_t high;
};

static_assert(sizeof(ipv6) == 16, "ipv6 struct padded incorrectly");

class Socket {
    friend class Poll;

#ifdef WIN32
    static std::atomic<bool> wsaInit;
#endif

protected:
    SOCKET sock;

    static bool wouldBlock();

    /**
     * Creates a new socket.
     *
     * @param ip6 If the socket uses IPv6
     * @param stream If the socket uses TCP (otherwise UDP.)
     */
    Socket(bool ip6, bool stream);

    /**
     * Wraps an existing socket
     *
     * @param sock Socket to wrap
     */
    explicit Socket(SOCKET sock);

public:
    static void init();
    static void deinit();

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    virtual ~Socket();

    virtual void close();

    virtual void process() = 0;
    virtual bool valid();
};
