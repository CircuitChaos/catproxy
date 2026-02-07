#pragma once

#include <string>
#include <cinttypes>
#include <vector>
#include <utility>

struct InterpolatorEntry {
	uint8_t raw;
	double numeric;
	std::string textual;
};

std::pair<double, std::string> interpolate(uint8_t raw, const std::vector<InterpolatorEntry> &cal);
