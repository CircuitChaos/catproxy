#pragma once

#include "fd.h"

class SignalFd: public Fd {
public:
	SignalFd();
	virtual ~SignalFd();

	/* Returns signal number */
	int read();
};
