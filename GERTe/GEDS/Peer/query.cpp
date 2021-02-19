#include "query.h"
#include <thread>
#include <chrono>
using namespace std;

void query(Address target) {
	// TODO: broadcast(string({8}) + target.tostring());
	this_thread::sleep_for((chrono::duration<int>) 5);
}
