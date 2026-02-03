#include "uiresources.h"

UiResources::~UiResources()
{
	if(image) {
		/* Fress also pixels */
		XDestroyImage(image);
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
