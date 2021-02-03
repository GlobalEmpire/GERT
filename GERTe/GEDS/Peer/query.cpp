#include "query.h"
#include <thread>
#include <chrono>
#include "peerManager.h"
#include "../Gateway/routeManager.h"
#include "../Networking/netty.h"
using namespace std;

void query(Address target) {
	Peer::broadcast(string({8}) + target.tostring());
	this_thread::sleep_for((chrono::duration<int>) 5);
}
