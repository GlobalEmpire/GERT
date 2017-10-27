#ifdef _WIN32
#define DLLExport __declspec(dllexport)
#define DLLImport __declspec(dllimport)
#else
#define DLLExport
#define DLLImport extern
#endif

#define STARTEXPORT extern "C" {
#define ENDEXPORT }
#define STARTIMPORT STARTEXPORT
#define ENDIMPORT ENDEXPORT

#define __DYNLIB__
#include "../Key.h"
#include "../Gateway.h"
#include "../Peer.h"
#include "../GERTc.h"
#include "../logging.h"
#include "NetString.h"
#include "./Tunnel.h"


STARTIMPORT
	DLLImport bool sendToGateway(Address, string);
	DLLImport bool isRemote(Address);
	DLLImport void setRoute(Address, Peer*);
	DLLImport void removeRoute(Address);
	DLLImport void addResolution(Address, Key);
	DLLImport void removeResolution(Address);
	DLLImport void addPeer(IP, Ports);
	DLLImport void removePeer(IP);
	DLLImport void broadcast(string);
	DLLImport bool queryWeb(Address);
ENDIMPORT
