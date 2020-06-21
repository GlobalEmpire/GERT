#pragma once
#include <string>

extern bool online_reporter;

namespace Sentry {
	void init();
	void stop();

	void crash(const std::string);
	void error(const std::string);
	void warn(const std::string);
}
