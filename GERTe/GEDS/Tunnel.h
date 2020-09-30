#pragma once
#include "GERTc.h"

struct Tunnel {
	GERTc local;
	GERTc remote;
	int remoteId;
};
