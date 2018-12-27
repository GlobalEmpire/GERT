/*
 * Manages peers and their associated connections for mapping use.
 */

#include <fcntl.h>
#include "peerManager.h"
#include "netty.h"
#include "logging.h"

using namespace std;

extern map<IP, Peer*> peers;
extern map<IP, KnownPeer> peerList;

extern volatile bool running;

bool peerIter::isEnd() { return (ptr == peers.end()) || (ptr == ++peers.end()); }
peerIter peerIter::operator++ (int a) { return (ptr++, *this); }
peerIter::peerIter() : ptr(peers.begin()) {};
Peer* peerIter::operator*() { return ptr->second; }

bool knownIter::isEnd() { return ptr == peerList.end(); }
knownIter knownIter::operator++ (int a) { return (ptr++, *this); }
knownIter::knownIter() : ptr(peerList.begin()) {};
KnownPeer knownIter::operator*() { return ptr->second; }
