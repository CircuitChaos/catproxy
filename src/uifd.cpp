#include <cstring>
#include "uifd.h"
#include "throw.h"
#include "log.h"
#include "font.h"

static const unsigned MIN_WIDTH                = 30 * font::SCALED_CHAR_WIDTH;
static const unsigned MAX_WIDTH                = 4000;
static const unsigned STARTING_WIDTH           = MIN_WIDTH;
static const unsigned WINDOW_HEIGHT            = 6 * font::SCALED_CHAR_HEIGHT;
static const uint8_t BACKGROUND_SUBPIXEL_COLOR = 0x10;
static const uint32_t BACKGROUND_COLOR         = (BACKGROUND_SUBPIXEL_COLOR << 16) | (BACKGROUND_SUBPIXEL_COLOR << 8) | BACKGROUND_SUBPIXEL_COLOR;
static const char WIN_NAME[]                   = "CAT Proxy";

UiFd::UiFd()
{
	res.dpy = XOpenDisplay(nullptr);
	xassert(res.dpy, "XOpenDisplay() failed -- cannot open display");
	setFd(ConnectionNumber(res.dpy));

	const int screen = DefaultScreen(res.dpy);
	xassert(XMatchVisualInfo(res.dpy, screen, 24, TrueColor, &res.vi) != 0, "This program needs at least 24-bit display");

	res.colormap = XCreateColormap(res.dpy, DefaultRootWindow(res.dpy), res.vi.visual, AllocNone);
	XSetWindowAttributes attr;
	attr.colormap         = res.colormap;
	attr.background_pixel = 0;
	attr.border_pixel     = 0;

	width  = STARTING_WIDTH;
	height = WINDOW_HEIGHT;

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
	res.sizeHints->min_width  = MIN_WIDTH;
	res.sizeHints->max_width  = MAX_WIDTH;
	res.sizeHints->min_height = height;
	res.sizeHints->max_height = height;

	XSetWMNormalHints(res.dpy, res.win, res.sizeHints);

	XWindowAttributes a;
	XGetWindowAttributes(res.dpy, res.win, &a);
	xassert((unsigned) a.width == width && (unsigned) a.height == height, "X created a window with a wrong size");

	recreateImage(width);
	XMapWindow(res.dpy, res.win);

	// for poll() to work
	XFlush(res.dpy);
}

UiFd::~UiFd()
{
}

bool UiFd::read()
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

				recreateImage(e.xconfigure.width);
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

void UiFd::update(const Meters &meters_)
{
	meters = meters_;
	recreateImage(width);
	putImage();
}

void UiFd::recreateImage(unsigned newWidth)
{
	maybeReallocImage(newWidth);
	memset(res.pixels, BACKGROUND_SUBPIXEL_COLOR, width * height * 4);

	// TODO for testing
	// meters.alc.emplace(0xff);
	// meters.cmp.emplace(0xff);
	// meters.idd.emplace(0xff);
	// meters.pwr.emplace(250);
	// meters.swr.emplace(52);
	// meters.sig.emplace(0xff);

	const uint32_t sigColor = meters.sig ? 0x00ff00 : 0x404040;
	const uint32_t cmpColor = meters.cmp ? 0x00ff00 : 0x404040;
	const uint32_t alcColor = meters.alc ? 0x00ff00 : 0x404040;
	const uint32_t pwrColor = meters.pwr ? 0xffff00 : 0x404040;
	const uint32_t iddColor = meters.idd ? 0xffff00 : 0x404040;
	const uint32_t swrColor = meters.swr ? 0xff0000 : 0x404040;

	putText(0, 0, sigColor, BACKGROUND_COLOR, "Sig [");
	putText(0, 1, cmpColor, BACKGROUND_COLOR, "Cmp [");
	putText(0, 2, alcColor, BACKGROUND_COLOR, "Alc [");
	putText(0, 3, pwrColor, BACKGROUND_COLOR, "Pwr [");
	putText(0, 4, iddColor, BACKGROUND_COLOR, "Idd [");
	putText(0, 5, swrColor, BACKGROUND_COLOR, "Swr [");

	const unsigned widthInChars = width / font::SCALED_CHAR_WIDTH;
	// Human-readable text is 7 chars max
	const unsigned humanTextStart = widthInChars - 7;

	putText(humanTextStart - 2, 0, sigColor, BACKGROUND_COLOR, "]");
	putText(humanTextStart - 2, 1, cmpColor, BACKGROUND_COLOR, "]");
	putText(humanTextStart - 2, 2, alcColor, BACKGROUND_COLOR, "]");
	putText(humanTextStart - 2, 3, pwrColor, BACKGROUND_COLOR, "]");
	putText(humanTextStart - 2, 4, iddColor, BACKGROUND_COLOR, "]");
	putText(humanTextStart - 2, 5, swrColor, BACKGROUND_COLOR, "]");

	const unsigned progressStart = 5 * font::SCALED_CHAR_WIDTH;
	const unsigned progressLen   = (widthInChars - 5 - 7 - 2) * font::SCALED_CHAR_WIDTH;

	if(meters.sig) {
		putText(humanTextStart, 0, sigColor, BACKGROUND_COLOR, meters.sig->toHuman());
		drawBar(progressStart, progressLen, 0, meters.sig->getRaw());
	}

	if(meters.cmp) {
		putText(humanTextStart, 1, cmpColor, BACKGROUND_COLOR, meters.cmp->toHuman());
		drawBar(progressStart, progressLen, 1, meters.cmp->getRaw());
	}

	if(meters.alc) {
		putText(humanTextStart, 2, alcColor, BACKGROUND_COLOR, meters.alc->toHuman());
		drawBar(progressStart, progressLen, 2, meters.alc->getRaw());
	}

	if(meters.pwr) {
		putText(humanTextStart, 3, pwrColor, BACKGROUND_COLOR, meters.pwr->toHuman());
		drawBar(progressStart, progressLen, 3, meters.pwr->getRaw());
	}

	if(meters.idd) {
		putText(humanTextStart, 4, iddColor, BACKGROUND_COLOR, meters.idd->toHuman());
		drawBar(progressStart, progressLen, 4, meters.idd->getRaw());
	}

	if(meters.swr) {
		putText(humanTextStart, 5, swrColor, BACKGROUND_COLOR, meters.swr->toHuman());
		drawBar(progressStart, progressLen, 5, meters.swr->getRaw());
	}

	// TODO add peak point (white?)
}

void UiFd::maybeReallocImage(unsigned newWidth)
{
	xassert(newWidth > 0, "Refusing to create image with width = 0");

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

void UiFd::putImage()
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

void UiFd::putChar(unsigned cx, unsigned cy, uint32_t color, uint32_t backgr, char ch)
{
	for(size_t y = 0; y < font::SCALED_CHAR_HEIGHT; ++y) {
		bool lineBuf[font::SCALED_CHAR_WIDTH];
		font::getLine(lineBuf, ch, y);

		for(size_t x = 0; x < font::SCALED_CHAR_WIDTH; ++x) {
			setPixel(cx * font::SCALED_CHAR_WIDTH + x, cy * font::SCALED_CHAR_HEIGHT + y, lineBuf[x] ? color : backgr);
		}
	}
}

void UiFd::putText(unsigned x, unsigned y, uint32_t color, uint32_t backgr, const std::string &text)
{
	for(const char *ctext = text.c_str(); *ctext; ++ctext) {
		putChar(x++, y, color, backgr, *ctext);
	}
}

void UiFd::setPixel(unsigned x, unsigned y, uint32_t color)
{
	xassert(x < width && y < height, "Attempted to set pixel outside window boundaries (%u,%u, boundaries: %u,%u)", x, y, width, height);
	res.pixels[y * width + x] = color;
}

void UiFd::drawBar(unsigned xstart, unsigned xlen, unsigned yInChars, uint8_t raw)
{
	const unsigned ystart   = yInChars * font::SCALED_CHAR_HEIGHT + font::FONT_SCALE * 2;
	const unsigned ylen     = font::SCALED_CHAR_HEIGHT - font::FONT_SCALE * 6;
	const unsigned xreallen = xlen * raw / 255;

	for(unsigned y = 0; y < ylen; ++y) {
		for(unsigned x = 0; x < xreallen; ++x) {
			uint32_t color;
			if(x < xlen / 2) {
				/* First half goes from grey (0x404040) to blue (0x6060ff) */
				const uint8_t b = x * (0xff - 0x40) / (xlen / 2) + 0x40;
				color           = 0x404000 | b;
			}
			else {
				/* Second half goes from blue (0x4040ff) to white (0xffffff) */
				const uint8_t rg = (x - xlen / 2) * (0xff - 0x40) / (xlen / 2) + 0x40;
				color            = (rg << 16) | (rg << 8) | 0xff;
			}

			setPixel(x + xstart, y + ystart, color);
		}
	}
}
