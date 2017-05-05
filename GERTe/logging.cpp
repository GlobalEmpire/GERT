#include <string>
#include <iostream>
#include <time.h>
#include "config.h"
using namespace std;

string timeOut() {
	time_t rawtime;
	time(&rawtime);
	tm* curTime = localtime(&rawtime);
	return to_string(curTime->tm_hour) + ":" + to_string(curTime->tm_min) + ":" + to_string(curTime->tm_sec);
}

void log(string msg) {
	cout << "[I][" << timeOut() << "] " << msg << "\n";
}

void warn(string msg) {
	cout << "[W][" << timeOut() << "] " << msg << "\n";
}

void error(string msg) {
	cout << "[E][" << timeOut() << "] " << msg << "\n";
}

void debug(string msg) {
#if DEBUG == true
	cout << "[D][" << timeOut() << "] " << msg << "\n";
#endif
}
