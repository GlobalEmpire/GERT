#include "INet.h"
#include "Poll.h"

extern Poll netPoll;

#ifdef _WIN32
INet::INet(const INet& orig) noexcept : sock(orig.sock), type(orig.type) {}		// The lock cannot be safely relocked
#endif

INet::INet(INet::Type type) : type(type) {};

INet::~INet() { //Ensure any destruction of an INet releases the mutex and deregisters the socket
    netPoll.remove(sock);

#ifdef _WIN32
    lock.unlock();
#endif
}
