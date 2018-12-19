#pragma once
#include "Version.h"
#include "Gateway.h"
#include "Peer.h"
#include "logging.h"
#include "NetString.h"
#include "Key.h"
#include "GERTc.h"

extern bool sendToGateway(Address, string);
extern bool isRemote(Address);
extern void setRoute(Address, Peer*);
extern void removeRoute(Address);
extern void addResolution(Address, Key);
extern void removeResolution(Address);
extern void addPeer(IP, Ports);
extern void removePeer(IP);
extern void broadcast(std::string);
extern bool queryWeb(Address);
