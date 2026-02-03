#pragma once

// TODO make this configurable from a config file

#include <termios.h>

namespace config {

static const char RADIO_PORT[]      = "/dev/ttyFTCAT";
static const speed_t RADIO_BAUD     = B38400;
static const char SYMLINK_PATH[]    = "/tmp/ttyCATProxy";
static const unsigned CAT_TIMEOUT   = 5000; /* ms */
static const unsigned POLL_INTERVAL = 250;  /* ms */

} // namespace config
