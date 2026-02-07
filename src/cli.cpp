#include <unistd.h>
#include "cli.h"
#include "throw.h"

Cli::Cli(int argc, char *const argv[])
{
	int opt;
	while((opt = getopt(argc, argv, ":c:dhl")) != -1) {
		switch(opt) {
			case '?':
				xthrow("-%c: option not recognized", optopt);

			case ':':
				xthrow("-%c: option requires argument", optopt);

			case 'c':
				configFile = optarg;
				break;

			case 'd':
				debugFlag = true;
				break;

			case 'h':
				help();
				exitFlag = true;
				break;

			case 'l':
				listFlag = true;
				break;

			default:
				xthrow("Unknown return value from getopt(): %d", opt);
		}
	}

	xassert(optind == argc, "Excessive arguments on the command line");
}

bool Cli::getExit() const
{
	return exitFlag;
}

bool Cli::getList() const
{
	return listFlag;
}

bool Cli::getDebug() const
{
	return debugFlag;
}

const std::string &Cli::getConfigFile() const
{
	return configFile;
}

void Cli::help()
{
	static const char usage[] =
	    "Usage: catproxy [-dlh] [-c <config_file>]\n"
	    "\n"
	    "Options:\n"
	    "  -d: enable debug output\n"
	    "  -l: list supported radio models and outputs\n"
	    "  -h: show help (this screen)\n"
	    "  -c <config_file>: specify alternate config file\n"
	    "\n"
	    "Default config file is ~/catproxy.conf.\n";

	printf("%s", usage);
}
