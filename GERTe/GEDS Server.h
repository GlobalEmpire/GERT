#include <string> //Required for processing inputs

enum connType { //Define the connection types
	GATEWAY,
	GEDS
};

struct GERTaddr { //Define a GERT address number
	int high;
	int low;
};

struct connection { //Define a connection
	bool initiated; //Defines if a connection has been initiated, should be set to true after first process
	connType type; //Define what type of connection this is
	GERTaddr addr; //Define what GERTaddr is associated with this connection, can be empty if GEDS P2P
};