#pragma once

#include <inttypes.h>
#include <stddef.h>

namespace font {

static const size_t UNSCALED_CHAR_WIDTH  = 9;
static const size_t UNSCALED_CHAR_HEIGHT = 16;
static const size_t FONT_SCALE           = 1;
static const size_t SCALED_CHAR_WIDTH    = UNSCALED_CHAR_WIDTH * FONT_SCALE;
static const size_t SCALED_CHAR_HEIGHT   = UNSCALED_CHAR_HEIGHT * FONT_SCALE;

void getLine(bool line[SCALED_CHAR_WIDTH], char ch, size_t lineNo);

} // namespace font
