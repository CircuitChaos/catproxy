#include "catreader.h"
#include "throw.h"

static const size_t MAX_CAT_SIZE = 256; /* More than enough */

std::optional<std::string> CatReader::feed(uint8_t byte)
{
	command += byte;
	xassert(command.size() < MAX_CAT_SIZE, "Maximum CAT command size exceeded");

	if(byte == ';') {
		const std::string cpy(command);
		command.clear();
		return cpy;
	}

	return std::nullopt;
}
