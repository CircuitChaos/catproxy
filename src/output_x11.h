#include "output.h"
#include "config.h"
#include "x11resources.h"
#include "fd.h"

class OutputX11 : public Output {
public:
	OutputX11(const Config &conf, unsigned meterCount, unsigned maxCookedSize);
	virtual ~OutputX11() {}

	virtual int getFd() const;
	virtual bool read();
	virtual void update(const Meters &meters);

	static std::string getName();

private:
	const Config &conf;
	const unsigned confScale;      /* Scale from config, copied to speed up lookup */
	const uint32_t confBackground; /* Background color from config, copied to speed up lookup */
	const unsigned maxCookedSize;  /* Maximum size of cooked text (from model) */
	X11Resources res;              /* Various resources used by X11 */
	Meters meters;                 /* Meters from last update() call */
	unsigned width;                /* Current window width, in pixels */
	unsigned height;               /* Current window height, in pixels */
	Fd fd;                         /* X11 connection number (descriptor) */

	/* x and y are in pixels, rows and cols are in chars */

	void createImage(unsigned newWidth);
	void createMeter(unsigned row, const Meter &meter);
	void maybeReallocImage(unsigned newWidth);
	void putImage();
	void putChar(unsigned col, unsigned row, uint32_t fgcolor, uint32_t bgcolor, char ch);
	void putText(unsigned col, unsigned row, uint32_t fgcolor, uint32_t bgcolor, const std::string &text);
	void setPixel(unsigned x, unsigned y, uint32_t color);
	void drawBar(unsigned xstart, unsigned xlen, unsigned yInChars, uint8_t raw);
	uint32_t getSwrMeterColor(const std::string &cooked);

	static uint32_t interpolateColor(unsigned value, unsigned maxValue, uint32_t startColor, uint32_t endColor);
};
