#define _CRT_SECURE_NO_WARNINGS

#include "logging.h"
#include <time.h>
#include <iostream>
using namespace std;

extern bool debugMode;

FILE * logFile;

void startLog() {
	logFile = fopen("errors.log", "a");
}

string timeOut() {
	time_t rawtime;
	time(&rawtime);
	tm * curTime = localtime(&rawtime);

	char time[9];
	strftime(time, 9, "%T", curTime);

	return string{ time };
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

void error2(string msg) {
	string log = "[E][" + timeOut() + "] " + msg;
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
