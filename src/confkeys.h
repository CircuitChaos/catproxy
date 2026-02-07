#pragma once

namespace config {

/* Generate from config file with:
 *
 * for key in $(grep -v \; catproxy.conf.example | grep = | awk '{print $1}'); do \
 * key2=$(echo $key | tr a-z A-Z | sed 's/\./_/g'); \
 * echo "static const char $key2[] = \"$f\";"; \
 * done
 */

static const char PORT_DEVICE[]                             = "port.device";
static const char PORT_BAUDRATE[]                           = "port.baudrate";
static const char PTY_SYMLINK[]                             = "pty.symlink";
static const char CAT_POLL_INTERVAL[]                       = "cat.poll_interval";
static const char CAT_TIMEOUT[]                             = "cat.timeout";
static const char METERS_RADIO_MODEL[]                      = "meters.radio_model";
static const char OUTPUT_TYPE[]                             = "output.type";
static const char OUTPUT_X11_SCALE[]                        = "output.x11.scale";
static const char OUTPUT_X11_COLOR_BACKGROUND[]             = "output.x11.color.background";
static const char OUTPUT_X11_COLOR_DISABLED_METER[]         = "output.x11.color.disabled_meter";
static const char OUTPUT_X11_COLOR_METER_BASE[]             = "output.x11.color.meter.";
static const char OUTPUT_X11_COLOR_METER_SWR_LOW[]          = "output.x11.color.meter.swr.low";
static const char OUTPUT_X11_COLOR_METER_SWR_MEDIUM[]       = "output.x11.color.meter.swr.medium";
static const char OUTPUT_X11_COLOR_METER_SWR_HIGH[]         = "output.x11.color.meter.swr.high";
static const char OUTPUT_X11_COLOR_METER_SWR_THRES_MEDIUM[] = "output.x11.color.meter.swr.thres_medium";
static const char OUTPUT_X11_COLOR_METER_SWR_THRES_HIGH[]   = "output.x11.color.meter.swr.thres_high";
static const char OUTPUT_X11_COLOR_BAR_MINIMUM[]            = "output.x11.color.bar.minimum";
static const char OUTPUT_X11_COLOR_BAR_MEDIUM[]             = "output.x11.color.bar.medium";
static const char OUTPUT_X11_COLOR_BAR_MAXIMUM[]            = "output.x11.color.bar.maximum";

} // namespace config
