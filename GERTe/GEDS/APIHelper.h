#pragma once
#include "API.h"
#include "Version.h"
#include "Gateway.h"
#include "Peer.h"
#include "logging.h"
#include "NetString.h"
#include "Key.h"
#include "GERTc.h"
#include "gatewayManager.h"
#include "peerManager.h"
#include "routeManager.h"
#include "query.h"

extern void addResolution(Address, Key);
extern void removeResolution(Address);
