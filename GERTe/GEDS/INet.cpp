#include "INet.h"
#include "logging.h"
#include <windows.h>

#ifdef _WIN32
INet::INet(const INet& orig) noexcept : sock(orig.sock), type(orig.type) {}		// The lock cannot be safely relocked
#endif

INet::INet(INet::Type type) : type(type) {};
