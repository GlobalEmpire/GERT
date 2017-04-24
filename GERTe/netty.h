void startup();
void shutdown();
void runServer();
void addPeer(in_addr, portComplex);
void removePeer(in_addr);
void setRoute(GERTaddr, peer);
void removeRoute(GERTaddr);
void killConnections();
