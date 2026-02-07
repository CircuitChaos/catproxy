#include <unistd.h>
#include "fd.h"

Fd::~Fd()
{
	if(!noclose && fd >= 0) {
		close(fd);
	}
}

void Fd::setFd(int fd_, bool noclose_)
{
	reset(fd_, noclose_);
}

void Fd::reset(int fd_, bool noclose_)
{
	if(!noclose && fd >= 0) {
		close(fd);
	}

	fd = fd_;
	noclose = noclose_;
}

int Fd::getFd()
{
	return fd;
}

Fd::operator int() const
{
	return fd;
}
