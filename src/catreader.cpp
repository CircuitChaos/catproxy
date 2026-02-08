#include "catreader.h"
#include "log.h"

static const size_t MAX_CAT_SIZE = 256; /* More than enough */

std::optional<std::string> CatReader::feed(uint8_t byte)
{
	if(!overflow) {
		command += byte;

		if(command.size() >= MAX_CAT_SIZE) {
			logn("Warning: Maximum CAT command length exceeded, dropping this command");
			command.clear();
			overflow = true;
		}
	}

	if(byte == ';') {
		if(overflow) {
			overflow = false;
			return std::nullopt;
		}

		const std::string cpy(command);
		command.clear();
		return cpy;
	}

	return std::nullopt;
}
