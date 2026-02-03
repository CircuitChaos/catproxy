#pragma once

#include <termios.h>
#include <string>
#include "fd.h"

class PortFd: public Fd {
public:
	PortFd(const std::string &path, speed_t speed);
	virtual ~PortFd();
};
