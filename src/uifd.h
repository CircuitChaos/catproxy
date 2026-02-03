#pragma once

#include "fd.h"
#include "uiresources.h"
#include "meters.h"

class UiFd : public Fd {
public:
	UiFd();
	virtual ~UiFd();

	/* If false is returned then window has been closed -- terminate program */
	bool read();
	void update(const Meters &meters);

private:
	UiResources res;
	Meters meters;
	unsigned width, height;

	void recreateImage(unsigned newWidth);
	void maybeReallocImage(unsigned newWidth);
	void redrawImage();
	void putImage();
	// x and y in putChar() and putText() are in chars, not in pixels
	// internal color representation here is 0rgb
	void putChar(unsigned x, unsigned y, uint32_t color, uint32_t backgr, char ch);
	void putText(unsigned x, unsigned y, uint32_t color, uint32_t backgr, const std::string &text);
	void setPixel(unsigned x, unsigned y, uint32_t color);
	void drawBar(unsigned xstart, unsigned xlen, unsigned yInChars, uint8_t raw);
};
