#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <iostream>
#include <time.h>
using namespace std;

extern bool debugMode;

FILE * logFile;

void startLog() {
	logFile = fopen("errors.log", "a");
}

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
	string log = "[E][" + timeOut() + "] " + msg + "\n";
	cout << log;
	fputs(log.c_str(), logFile);
}

void debug(string msg) {
	if (debugMode)
		cout << "[D][" << timeOut() << "] " << msg << "\n";
}

void stopLog() {
	fclose(logFile);
}
