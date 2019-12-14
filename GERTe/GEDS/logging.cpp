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

void inline doPrint(const string msg, const char type, bool newline) {
	char* buf = new char[15 + msg.length()];


	if (newline)
		sprintf(buf, "[%c][%s] %s\n", type, timeOut().c_str(), msg.c_str());
	else
		sprintf(buf, "[%c][%s] %s", type, timeOut().c_str(), msg.c_str());

	if (type == 'E')
		cerr << buf;
	else
		cout << buf;

	if (logFile != nullptr)
		fputs(buf, logFile);

	delete[] buf;
}

void log(string msg, bool newline) {
	doPrint(msg, 'I', newline);
}

void warn(string msg, bool newline) {
	doPrint(msg, 'W', newline);
}

void error(string msg, bool newline) {
	doPrint(msg, 'E', newline);
}

void debug(string msg, bool newline) {
	if (debugMode)
		doPrint(msg, 'D', newline);
}

void stopLog() {
	fputs("-----END FILE-----\n", logFile);
	fclose(logFile);
}
