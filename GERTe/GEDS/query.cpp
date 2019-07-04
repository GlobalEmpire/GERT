#include "query.h"
#include "netty.h"
#include "Peer.h"
#include <thread>
#include <chrono>
using namespace std;

void query(Address target) {
	Peer::broadcast(string({8}) + target.tostring());
	this_thread::sleep_for((chrono::duration<int>) 5);
}
