#include <string>
#include <iostream>
#include <time.h>
using namespace std;

extern bool debugMode;

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
	if (debugMode)
		cout << "[D][" << timeOut() << "] " << msg << "\n";
}
