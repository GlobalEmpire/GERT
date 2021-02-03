/*
 * Manages peers and their associated connections for mapping use.
 */

#include "peerManager.h"
using namespace std;

extern map<IP, Peer*> peers;

bool peerIter::isEnd() { return (ptr == peers.end()) || (ptr == peers.end()); }
peerIter peerIter::operator++ (int a) { ptr++; return (*this); }
peerIter::peerIter() : ptr(peers.begin()) {};
Peer* peerIter::operator*() { return ptr->second; }
