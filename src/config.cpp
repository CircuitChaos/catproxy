#include <ini.h> // libinih: https://github.com/benhoyt/inih
#include <climits>
#include "throw.h"
#include "config.h"

int configHandler(void *user, const char *section, const char *name, const char *value)
{
	xassert(user && section && name && value, "One of arguments in a libinih callback is null");
	xassert(!*section, "Sections are not supported");

	Config *config = (Config *) user;
	config->add(name, value);

	return 1;
}

Config::Config(const std::string &file)
{
	const int rs = ini_parse(file.c_str(), configHandler, this);
	xassert(rs != -1, "Could not open config file %s", file.c_str());
	xassert(rs >= 0, "ini_parse() should return >= 0 or -1, returned %d instead", rs);
	xassert(rs == 0, "Error parsing config file %s at line %d", file.c_str(), rs);
}

void Config::add(const std::string &key, const std::string &value)
{
	xassert(!exists(key), "Duplicate config key %s", key.c_str());
	data[key] = value;
}

bool Config::exists(const std::string &key) const
{
	return data.find(key) != data.end();
}

const std::string &Config::getString(const std::string &key) const
{
	const std::map<std::string, std::string>::const_iterator i = data.find(key);
	xassert(i != data.end(), "Mandatory config key %s doesn't exist", key.c_str());
	return i->second;
}

unsigned Config::getInt(const std::string &key) const
{
	const std::string v = getString(key);
	const long l        = strtol(v.c_str(), nullptr, 10);
	xassert(l >= 0, "Negative value for key %s not supported", key.c_str());
	xassert((unsigned long) l <= UINT_MAX, "Value for key %s is too large", key.c_str());
	return l;
}

float Config::getFloat(const std::string &key) const
{
	const std::string v = getString(key);
	return strtof(v.c_str(), nullptr);
}

uint32_t Config::getColor(const std::string &key) const
{
	const std::string v = getString(key);
	const long l        = strtol(v.c_str(), nullptr, 16);
	xassert(l <= 0xffffff, "Hex value for the color %s is too large", key.c_str());
	return l;
}
