#include <cstdarg>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include "util.h"
#include "throw.h"
#include "log.h"

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

std::string util::applyUserHome(const std::string &path)
{
	const char *home = getenv("HOME");
	xassert(home && *home, "Could not find user's home directory");

	/* TODO this could be done more efficiently, revisit */
	std::string out;
	for(std::string::const_iterator i = path.begin(); i != path.end(); ++i) {
		if(i == path.begin() && *i == '~') {
			out += home;
		}
		else {
			out += *i;
		}
	}
	return out;
}

std::string util::toLower(const std::string &s)
{
	std::string rs(s);
	std::transform(rs.begin(), rs.end(), rs.begin(), [](char ch) { return std::tolower(ch); });
	return rs;
}
