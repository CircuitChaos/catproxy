#pragma once

#include <string>
#include <vector>
#include <cinttypes>

struct Meter {
	bool available;
	bool isSwr; // has special treatment in the UI
	std::string name;
	uint8_t raw;
	std::string cooked;
};

typedef std::vector<Meter> Meters;
