#pragma once

#include <string>
#include <set>
#include <map>
#include <vector>
#include "config.h"
#include "meters.h"

class Output {
public:
	virtual ~Output() {}

	/* There might be one fd that's polled for reading */
	virtual int getFd() const { return -1; }

	/* Called only if getFd() >= 0. Return false to terminate app */
	virtual bool read() { return true; }

	virtual void update(const Meters &meters) = 0;
};

typedef std::set<std::string> OutputList;
OutputList getOutputList();
Output *createOutput(const std::string &name, const Config &conf, unsigned meterCount, unsigned maxCookedSize);
