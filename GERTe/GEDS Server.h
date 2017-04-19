#include <string> //Required for processing inputs

typedef unsigned char UCHAR;
typedef unsigned short ushort;

enum connType { //Define the connection types
	GATEWAY,
	GEDS
};

struct GERTaddr { //Define a GERT address number
	unsigned short high;
	unsigned short low;
};

struct connection { //Define a connection
	UCHAR state; //Defines if a connection has been initiated, should be set to true after first process
	connType type; //Define what type of connection this is
	GERTaddr addr; //Define what GERTaddr is associated with this connection, can be empty if GEDS P2P
};

struct GERTkey {
	char key[20];
};

void sendTo(GERTaddr, string);
void sendTo(connection, string);
void closeConnection(connection);
bool assign(connection, GERTaddr, GERTkey);
void addResolution(GERTaddr, GERTkey);
void removeResolution(GERTaddr);
void addPeer(char*, ushort, ushort);
void removePeer(char*);
void setRoute(GERTaddr, connection);
void removeRoute(GERTaddr);