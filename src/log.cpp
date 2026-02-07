#include <ctime>
#include <cstdarg>
#include "log.h"

static bool loggerDebug = false;

void loggerLog(bool debug, const char *file, int line, const char *fmt, ...)
{
	char tag = 'N';

	if(debug) {
		if(!loggerDebug) {
			return;
		}

		tag = 'D';
	}

	const time_t t = time(nullptr);

	tm tm;
	localtime_r(&t, &tm);

	char ts[64];
	strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm);

	printf("%s %c %s:%d: ", ts, tag, file, line);

	va_list ap;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	printf("\n");
}

void loggerSetDebug(bool debug)
{
	loggerDebug = debug;
}
