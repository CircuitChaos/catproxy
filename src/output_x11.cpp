#include <cstring>
#include <cstdlib>
#include "output_x11.h"
#include "throw.h"
#include "log.h"
#include "font.h"
#include "confkeys.h"
#include "util.h"

static const unsigned MAX_WIDTH = 8000; // arbitrarily chosen
static const char WIN_NAME[]    = "CAT Proxy";

OutputX11::OutputX11(const Config &conf, unsigned meterCount, unsigned maxCookedSize)
    : conf(conf),
      confScale(conf.getInt(config::OUTPUT_X11_SCALE)),
      confBackground(conf.getColor(config::OUTPUT_X11_COLOR_BACKGROUND)),
      maxCookedSize(maxCookedSize)
{
	res.dpy = XOpenDisplay(nullptr);
	xassert(res.dpy, "XOpenDisplay() failed -- cannot open display");
	fd.reset(ConnectionNumber(res.dpy), true);

	const int screen = DefaultScreen(res.dpy);
	xassert(XMatchVisualInfo(res.dpy, screen, 24, TrueColor, &res.vi) != 0, "This program needs at least 24-bit display");

	res.colormap = XCreateColormap(res.dpy, DefaultRootWindow(res.dpy), res.vi.visual, AllocNone);
	XSetWindowAttributes attr;
	attr.colormap         = res.colormap;
	attr.background_pixel = 0;
	attr.border_pixel     = 0;

	const unsigned minWidth = 30 * font::WIDTH * confScale;
	width                   = minWidth;
	height                  = meterCount * font::HEIGHT * confScale;

	res.win = XCreateWindow(
	    res.dpy,                                  // display
	    DefaultRootWindow(res.dpy),               // parent
	    0, 0,                                     // initial position
	    width,                                    // initial width
	    height,                                   // initial height
	    0,                                        // border width
	    res.vi.depth,                             // depth
	    InputOutput,                              // class
	    res.vi.visual,                            // visual
	    CWColormap | CWBackPixel | CWBorderPixel, // value mask
	    &attr);                                   // attributes
	xassert(res.win, "XCreateWindow() failed");

	res.winDelMsg = XInternAtom(res.dpy, "WM_DELETE_WINDOW", false);
	xassert(res.winDelMsg != None, "XInternAtom() failed -- could not obtain WM_DELETE_WINDOW atom");

	XSetWMProtocols(res.dpy, res.win, &res.winDelMsg, 1);
	XStoreName(res.dpy, res.win, WIN_NAME);
	XSelectInput(res.dpy, res.win, ExposureMask | StructureNotifyMask);

	res.sizeHints = XAllocSizeHints();
	xassert(res.sizeHints, "XAllocSizeHints() failed");
	res.sizeHints->flags      = PMinSize | PMaxSize;
	res.sizeHints->min_width  = minWidth;
	res.sizeHints->max_width  = MAX_WIDTH;
	res.sizeHints->min_height = height;
	res.sizeHints->max_height = height;

	XSetWMNormalHints(res.dpy, res.win, res.sizeHints);

	XWindowAttributes a;
	XGetWindowAttributes(res.dpy, res.win, &a);
	xassert((unsigned) a.width == width && (unsigned) a.height == height, "X created a window with a wrong size");

	createImage(width);
	XMapWindow(res.dpy, res.win);

	// for poll() to work
	XFlush(res.dpy);
}

int OutputX11::getFd() const
{
	return fd;
}

bool OutputX11::read()
{
	while(XPending(res.dpy) > 0) {
		XEvent e;
		XNextEvent(res.dpy, &e);

		logd("Got X event of type %d", e.type);

		switch(e.type) {
			case Expose:    // ExposureMask
			case MapNotify: // StructureNotifyMask
				putImage();
				break;

			case ConfigureNotify: // StructureNotifyMask
				xassert(e.xconfigure.type == ConfigureNotify, "Invalid event type: %d", e.xconfigure.type);
				xassert((int) height == e.xconfigure.height, "Window resized vertically (%u -> %d), this shouldn't be allowed", height, e.xconfigure.height);
				if((int) width == e.xconfigure.width) {
					break;
				}

				createImage(e.xconfigure.width);
				putImage();
				break;

			case ClientMessage: // from WM
				xassert(e.xclient.type == ClientMessage, "Invalid event type: %d", e.xclient.type);
				if((Atom) e.xclient.data.l[0] == res.winDelMsg) {
					return false;
				}
				break;

			default:
				break;
		}
	}

	// for poll() to work
	XFlush(res.dpy);
	return true;
}

void OutputX11::update(const Meters &meters_)
{
	meters = meters_;
	createImage(width);
	putImage();
}

std::string OutputX11::getName()
{
	return "x11";
}

void OutputX11::createImage(unsigned newWidth)
{
	maybeReallocImage(newWidth);
	for(unsigned ofs = 0; ofs < width * height; ++ofs) {
		res.pixels[ofs] = confBackground;
	}

	for(unsigned row = 0; row < meters.size(); ++row) {
		createMeter(row, meters[row]);
	}
}

void OutputX11::createMeter(unsigned row, const Meter &meter)
{
	logd("Creating meter %s in row %u", meter.name.c_str(), row);

	const std::string lcname = util::toLower(meter.name);
	uint32_t color;
	if(!meter.available) {
		color = conf.getColor(config::OUTPUT_X11_COLOR_DISABLED_METER);
	}
	else if(meter.isSwr) {
		color = getSwrMeterColor(meter.cooked);
	}
	else {
		color = conf.getColor(std::string(config::OUTPUT_X11_COLOR_METER_BASE) + lcname);
	}

	const unsigned widthInChars    = width / (font::WIDTH * confScale);
	const unsigned cookedTextStart = widthInChars - maxCookedSize;

	putText(0, row, color, confBackground, util::format("%s [", meter.name.c_str()));
	putText(cookedTextStart - 2, row, color, confBackground, util::format("] %s", meter.cooked.c_str()));

	const unsigned barStart = (meter.name.size() + 2) * font::WIDTH * confScale;
	const unsigned barLen   = (widthInChars - (meter.name.size() + 2) - maxCookedSize - 2) * font::WIDTH * confScale;
	drawBar(barStart, barLen, row, meter.raw);
}

uint32_t OutputX11::getSwrMeterColor(const std::string &cooked)
{
	const float swr = strtof(cooked.c_str(), nullptr);

	if(swr > conf.getFloat(config::OUTPUT_X11_COLOR_METER_SWR_THRES_HIGH)) {
		return conf.getColor(config::OUTPUT_X11_COLOR_METER_SWR_HIGH);
	}

	if(swr > conf.getFloat(config::OUTPUT_X11_COLOR_METER_SWR_THRES_MEDIUM)) {
		return conf.getColor(config::OUTPUT_X11_COLOR_METER_SWR_MEDIUM);
	}

	return conf.getColor(config::OUTPUT_X11_COLOR_METER_SWR_LOW);
}

void OutputX11::maybeReallocImage(unsigned newWidth)
{
	xassert(newWidth > 0, "Refusing to create image with zero width");

	if(newWidth != width) {
		if(res.image) {
			XDestroyImage(res.image);
			res.image = nullptr;
		}

		width = newWidth;
	}

	if(!res.image) {
		res.pixels = (uint32_t *) malloc(width * height * 4);
		xassert(res.pixels, "malloc() failed: %m");

		res.image = XCreateImage(
		    res.dpy,             // display
		    res.vi.visual,       // visual
		    res.vi.depth,        // depth
		    ZPixmap,             // format
		    0,                   // offset
		    (char *) res.pixels, // data
		    width,               // width
		    height,              // height
		    32,                  // bitmap pad -- not sure
		    0);                  // bytes per line
	}
}

void OutputX11::putImage()
{
	if(!res.image) {
		return;
	}

	XPutImage(
	    res.dpy,                                    // display
	    res.win,                                    // drawable
	    DefaultGC(res.dpy, DefaultScreen(res.dpy)), // gc
	    res.image,                                  // image
	    0, 0,                                       // src_x, src_y
	    0, 0,                                       // dst_x, dst_y
	    width, height);                             // width, height
}

void OutputX11::putChar(unsigned col, unsigned row, uint32_t fgcolor, uint32_t bgcolor, char ch)
{
	for(unsigned y = 0; y < font::HEIGHT * confScale; ++y) {
		bool lineBuf[font::WIDTH * confScale];
		font::getLine(confScale, lineBuf, ch, y);

		for(unsigned x = 0; x < font::WIDTH * confScale; ++x) {
			setPixel(col * font::WIDTH * confScale + x, row * font::HEIGHT * confScale + y, lineBuf[x] ? fgcolor : bgcolor);
		}
	}
}

void OutputX11::putText(unsigned col, unsigned row, uint32_t fgcolor, uint32_t bgcolor, const std::string &text)
{
	logd("putText() called, col=%u, row=%u, text=%s", col, row, text.c_str());

	for(const char *ctext = text.c_str(); *ctext; ++ctext) {
		putChar(col++, row, fgcolor, bgcolor, *ctext);
	}
}

void OutputX11::setPixel(unsigned x, unsigned y, uint32_t color)
{
	xassert(x < width && y < height, "Attempted to put pixel outside window boundaries (%u,%u, boundaries: %u,%u)", x, y, width, height);
	res.pixels[y * width + x] = color;
}

void OutputX11::drawBar(unsigned xstart, unsigned xlen, unsigned row, uint8_t raw)
{
	// xstart=45, xlen=144, row=5, raw=35
	//		45,96, boundaries: 270,96
	logd("drawBar() called, xstart=%u, xlen=%u, row=%u, raw=%u", xstart, xlen, row, raw);

	const unsigned ystart   = (row * font::HEIGHT + 3) * confScale;
	const unsigned ylen     = (font::HEIGHT - 8) * confScale;
	const unsigned xreallen = xlen * raw / 255;

	/* For faster access */
	const uint32_t colorMin = conf.getColor(config::OUTPUT_X11_COLOR_BAR_MINIMUM);
	const uint32_t colorMed = conf.getColor(config::OUTPUT_X11_COLOR_BAR_MEDIUM);
	const uint32_t colorMax = conf.getColor(config::OUTPUT_X11_COLOR_BAR_MAXIMUM);

	for(unsigned y = 0; y < ylen; ++y) {
		for(unsigned x = 0; x < xreallen; ++x) {
			uint32_t color;

			if(x < xlen / 2) {
				color = interpolateColor(x, xlen / 2, colorMin, colorMed);
			}
			else {
				color = interpolateColor(x - xlen / 2, xlen / 2, colorMed, colorMax);
			}

			setPixel(x + xstart, y + ystart, color);
		}
	}
}

uint32_t OutputX11::interpolateColor(unsigned value, unsigned maxValue, uint32_t startColor, uint32_t endColor)
{
	if(maxValue == 0) {
		/* Can't calculate */
		return startColor;
	}

	uint8_t startRgb[3] = {(uint8_t) ((startColor >> 16) & 0xff), (uint8_t) ((startColor >> 8) & 0xff), (uint8_t) (startColor & 0xff)};
	uint8_t endRgb[3]   = {(uint8_t) ((endColor >> 16) & 0xff), (uint8_t) ((endColor >> 8) & 0xff), (uint8_t) (endColor & 0xff)};
	uint8_t rgb[3];

	for(size_t i = 0; i < 3; ++i) {
		/* Note that these values can be negative */
		const int16_t colorDiffMax = endRgb[i] - startRgb[i];
		const int16_t colorDiffCur = value * colorDiffMax / maxValue;

		rgb[i] = startRgb[i] + colorDiffCur;
	}

	return (rgb[0] << 16) | (rgb[1] << 8) | rgb[2];
}
