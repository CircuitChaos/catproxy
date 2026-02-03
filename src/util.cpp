#include <cstdarg>
#include <cstdlib>
#include "util.h"
#include "throw.h"

std::string util::format(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	char *p;
	xassert(vasprintf(&p, fmt, ap) != -1, "Error allocating memory");
	va_end(ap);

	const std::string s(p);
	free(p);

	return s;
}
