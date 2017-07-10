#include "query.h"
#include <thread>
#include <chrono>
using namespace std;

bool queryWeb(GERTaddr target) {
	broadcast(string({ 8, putAddr(target) }));
	this_thread::sleep_for((chrono::duration<int>) 5);
	return isRemote(target);
}
