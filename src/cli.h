#pragma once

#include <string>

class Cli {
public:
	Cli(int argc, char *const argv[]);

	bool getExit() const;
	bool getList() const;
	bool getDebug() const;
	const std::string &getConfigFile() const;

private:
	bool exitFlag{false};
	bool listFlag{false};
	bool debugFlag{false};
	std::string configFile;

	void help();
};
