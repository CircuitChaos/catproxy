#include "catreader.h"

std::optional<std::string> CatReader::feed(uint8_t byte)
{
	// TODO add some limit on the size
	command += byte;

	if(byte == ';') {
		const std::string cpy(command);
		command.clear();
		return cpy;
	}

	return std::nullopt;
}
