#pragma once

#include <optional>
#include <string>
#include <cstdint>

class CatReader {
public:
	/* Feed one byte; if complete CAT command was read, returns the command */
	std::optional<std::string> feed(uint8_t byte);

private:
	std::string command;
	bool overflow{false};
};
