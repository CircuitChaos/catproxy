#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "portfd.h"
#include "throw.h"
#include "log.h"

PortFd::PortFd(const std::string &path, speed_t speed)
{
	const int fd = open(path.c_str(), O_RDWR | O_NOCTTY);
	xassert(fd >= 0, "open(%s) failed: %m", path.c_str());
	setFd(fd);

	termios t;
	xassert(tcgetattr(fd, &t) != -1, "tcgetattr() failed: %m");

	t.c_iflag = 0;
	t.c_oflag = 0;
	t.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	t.c_cflag &= ~(CSIZE | PARENB);
	t.c_cflag |= CS8;

	t.c_cc[VMIN]  = 1;
	t.c_cc[VTIME] = 0;

	xassert(cfsetispeed(&t, speed) != -1, "cfsetispeed() failed: %m");
	xassert(cfsetospeed(&t, speed) != -1, "cfsetospeed() failed: %m");
	xassert(tcsetattr(fd, TCSAFLUSH, &t) != -1, "tcsetattr() failed: %m");

	logd("Port %s opened and configured", path.c_str());
}

PortFd::~PortFd()
{
	tcdrain(getFd());
}
