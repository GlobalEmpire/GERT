#include "Sentry.h"
#include "logging.h"
#include "Versioning.h"
#include <sentry.h>

bool online_reporter = false;

void genericEvent(const std::string title, sentry_level_e level) {
	sentry_value_t event = sentry_value_new_event();

	sentry_set_level(level);
	sentry_value_set_by_key(event, "message", sentry_value_new_string(title.c_str()));

	sentry_capture_event(event);
}

void Sentry::init() {
	if (!online_reporter)
		return;
	
	sentry_options_t* options = sentry_options_new();
	sentry_options_set_dsn(options, "https://01df8c2ae1c8499297b5a0786cd03507@o342668.ingest.sentry.io/2510948");
	sentry_options_set_release(options, ThisVersion.stringify().c_str());
	sentry_init(options);
}

void Sentry::stop() {
	if (!online_reporter)
		return;

	sentry_shutdown();
}

void Sentry::crash(const std::string title) {
	if (!online_reporter)
		return;

	genericEvent(title, SENTRY_LEVEL_FATAL);

	Sentry::stop();
}

void Sentry::error(const std::string title) {
	if (!online_reporter)
		return;

	genericEvent(title, SENTRY_LEVEL_ERROR);
}

void Sentry::warn(const std::string title) {
	if (!online_reporter)
		return;

	genericEvent(title, SENTRY_LEVEL_WARNING);
}
