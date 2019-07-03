#pragma once

#ifdef _WIN32
typedef unsigned long long SOCKET;
static unsigned long nonZero = 1;
#else
typedef int SOCKET;
#endif

/*
	----- INet Generic Network Object -----
	The INet interface allows generic network objects to listen for and receive/process events simplistically.
	Any INet object can be placed into a poll. The poll will call process to process a relevant event.
*/

class INet {								// Generic processible network object
public:
	enum class Type {						// Possible types of INet objects
		LISTEN,
		CONNECT
	};

	SOCKET sock = -1;						// Associated socket
	Type type;								// The type of the INet object

	INet(INet::Type type) : type(type) {};	// Constructor for generic INet objects to force proper initalization
	virtual ~INet() = default;				// Virtual destructor to allow appropriate cleanup regardless of context

	virtual void process() = 0;				// Process network event
};
