#pragma once

#include <string>
#include <map>
#include <cinttypes>

class Config {
public:
	Config(const std::string &file);

	/* Use keys from confkeys here */
	bool exists(const std::string &key) const;
	const std::string &getString(const std::string &key) const;
	unsigned getInt(const std::string &key) const;
	float getFloat(const std::string &key) const;

	/* Special getter for hex-encoded color */
	uint32_t getColor(const std::string &key) const;

private:
	std::map<std::string, std::string> data;

	friend int configHandler(void *user, const char *section, const char *name, const char *value);
	void add(const std::string &key, const std::string &value);
};
