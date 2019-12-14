#pragma once
#include <string>

void log(std::string, bool = true);
void warn(std::string, bool = true);
void error(std::string, bool = true);
void debug(std::string, bool = true);
void startLog();
void stopLog();
