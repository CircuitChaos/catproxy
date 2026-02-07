#pragma once

#include <termios.h>
#include <string>
#include "fd.h"

class PortFd : public Fd {
public:
	PortFd(const std::string &path, unsigned baud);
	virtual ~PortFd();

private:
	static speed_t baudToSpeed(unsigned baud);
};
