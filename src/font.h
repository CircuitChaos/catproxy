#pragma once

#include <inttypes.h>
#include <stddef.h>

namespace font {

static const unsigned WIDTH  = 9;
static const unsigned HEIGHT = 16;

/* Line is WIDTH * scale bools long
 * lineNo is 0 to HEIGHT * scale - 1
 */
void getLine(unsigned scale, bool *line, char ch, unsigned lineNo);

} // namespace font
