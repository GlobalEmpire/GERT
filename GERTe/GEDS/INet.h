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

class INet {							// Generic processible network object
public:
	SOCKET sock;						// Associated socket

	virtual void process() = 0;			// Process network event
};
