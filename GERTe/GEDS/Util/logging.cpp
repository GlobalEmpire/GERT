#define _CRT_SECURE_NO_WARNINGS
#include "logging.h"
#include <ctime>
#include <iostream>
#include <mutex>
using namespace std;

extern bool debugMode;

FILE * logFile;

std::mutex log_lock;

void startLog() {
	logFile = fopen("errors.log", "a");
}

string timeOut() {
	time_t rawtime;
	time(&rawtime);
	tm* curTime = localtime(&rawtime);

	char time[9];
	strftime(time, 9, "%T", curTime);

	return string{ time };
}

void log(const string& msg) {
    std::lock_guard guard{ log_lock };
	cout << "[I][" << timeOut() << "] " << msg << endl;
}

void warn(const string& msg) {
    std::lock_guard guard{ log_lock };
	cout << "[W][" << timeOut() << "] " << msg << endl;
}

void error(const string& msg) {
    std::lock_guard guard{ log_lock };
	string log = "[E][" + timeOut() + "] " + msg + "\n";

	cout << log;
	cout.flush();

	fputs(log.c_str(), logFile);
}

void error2(const string& msg) {
    std::lock_guard guard{ log_lock };
	string log = "[E][" + timeOut() + "] " + msg;

	cout << log;
	cout.flush();

	fputs(log.c_str(), logFile);
}

void debug(const string& msg) {
	if (debugMode) {
        std::lock_guard guard{ log_lock };
        cout << "[D][" << timeOut() << "] " << msg << endl;
    }
}

void stopLog() {
    std::lock_guard guard{ log_lock };
	fclose(logFile);
}
