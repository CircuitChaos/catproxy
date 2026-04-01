#include <cstdlib>
#include "x11resources.h"

X11Resources::~X11Resources()
{
	if(image) {
		/* Fress also pixels */
		XDestroyImage(image);
	}
	else {
		/* In rare case, pixels might be already allocated */
		if(pixels) {
			free(pixels);
		}
	}

	if(sizeHints) {
		XFree(sizeHints);
	}

	if(win) {
		XDestroyWindow(dpy, win);
	}

	// TODO should we also free winDelMsg here?

	if(colormap != None) {
		XFreeColormap(dpy, colormap);
	}

	if(dpy) {
		XCloseDisplay(dpy);
	}
}
