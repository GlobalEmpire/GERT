#define _CRT_SECURE_NO_WARNINGS

#include "logging.h"
#include <time.h>
#include <iostream>
using namespace std;

extern bool debugMode;

FILE * logFile = nullptr;

void startLog() {
	logFile = fopen("geds.log", "a");
	fputs("-----START LOG-----\n", logFile);
}

string inline timeOut() {
	time_t rawtime;
	time(&rawtime);
	tm * curTime = localtime(&rawtime);

	char time[9];
	strftime(time, 9, "%T", curTime);

	return string{ time };
}

void inline doPrint(const string msg, const char type) {
	char* buf = new char[15 + msg.length()];

	sprintf(buf, "[%c][%s] %s\n", type, timeOut().c_str(), msg.c_str());

	if (type == 'E')
		cerr << buf;
	else
		cout << buf;

	if (logFile != nullptr)
		fputs(buf, logFile);
}

void log(string msg) {
	doPrint(msg, 'I');
}

void warn(string msg) {
	doPrint(msg, 'W');
}

void error(string msg) {
	doPrint(msg, 'E');
}

void error2(string msg) {
	string log = "[E][" + timeOut() + "] " + msg;
	cerr << log;
	fputs(log.c_str(), logFile);
}

void debug(string msg) {
	if (debugMode)
		doPrint(msg, 'D');
}

void stopLog() {
	fputs("-----END FILE-----\n", logFile);
	fclose(logFile);
}
