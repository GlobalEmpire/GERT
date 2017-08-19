#include "netDefs.h"

class Gateway : public connection {
	public:
		Address addr;
		void process(string data) { api->procGate(this, data); };
		void kill() { api->killGate(this); };
		Gateway(void* socket);
};
