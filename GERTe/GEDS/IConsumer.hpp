#include "INet.h"

/*
	----- IConsumer Generic Network Object -----
	The IConsumer interface allows network objects to consume a known or semi-known amount of data asyncronously and uniformly.
	The IConsumer interface implements the INet interface to receive it's events from a poll.
	Derived objects can call consume with the known amount of data, along with a boolean to signify a NetString appended.
	If Consume returns true, buf has all the needed data. Otherwise, the derived object needs to await another event.
	Furthermore, to allow the data to contain nulls, the total size of the buffer will be stored at bufsize.
	
	Note: buf must be freed when done being used by derived objects.
*/

class IConsumer : public INet {
	int lastbufsize = 0;					// Contains the size of the last buffer.
	char* lastbuf = nullptr;				// Contains the last buffer when it must be stored independently.

protected:
	IConsumer();							// Constructs the IConsumer, which must construct the underlying INet.

	bool consume(int, bool = false);		// Reads data off the socket, stores it, and returns if it has received all the data needed.
	void clean();							// Cleans up the current buffer. A shortcut for derived objects.

public:
	int bufsize = 0;						// Contains the size of the current buffer.
	char* buf = nullptr;					// Contains the current buffer or nullptr if there is none.
};