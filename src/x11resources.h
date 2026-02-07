#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <optional>
#include <cstdint>

struct X11Resources {
	~X11Resources();

	Display *dpy{nullptr};
	XVisualInfo vi;
	Colormap colormap{None};
	Atom winDelMsg{None};
	Window win{None};
	XSizeHints *sizeHints{nullptr};
	uint32_t *pixels{nullptr};
	XImage *image{nullptr};
};
