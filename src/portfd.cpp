#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include "portfd.h"
#include "throw.h"
#include "log.h"

PortFd::PortFd(const std::string &path, unsigned baud)
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

	const speed_t speed = baudToSpeed(baud);

	xassert(cfsetispeed(&t, speed) != -1, "cfsetispeed() failed: %m");
	xassert(cfsetospeed(&t, speed) != -1, "cfsetospeed() failed: %m");
	xassert(tcsetattr(fd, TCSAFLUSH, &t) != -1, "tcsetattr() failed: %m");

	logd("Port %s opened and configured", path.c_str());
}

PortFd::~PortFd()
{
	tcdrain(getFd());
}

speed_t PortFd::baudToSpeed(unsigned baud)
{
	static const std::map<unsigned, speed_t> map = {
	    {50, B50},
	    {75, B75},
	    {110, B110},
	    {134, B134},
	    {150, B150},
	    {200, B200},
	    {300, B300},
	    {600, B600},
	    {1200, B1200},
	    {1800, B1800},
	    {2400, B2400},
	    {4800, B4800},
	    {9600, B9600},
	    {19200, B19200},
	    {38400, B38400},
	    {57600, B57600},
	    {115200, B115200},
	    {230400, B230400},
	    {460800, B460800},
	    {500000, B500000},
	    {576000, B576000},
	    {921600, B921600},
	    {1000000, B1000000},
	    {1152000, B1152000},
	    {1500000, B1500000},
	    {2000000, B2000000},
	    {2500000, B2500000},
	    {3000000, B3000000},
	    {3500000, B3500000},
	    {4000000, B4000000},
	};

	const std::map<unsigned, speed_t>::const_iterator i = map.find(baud);
	xassert(i != map.end(), "Baudrate %u is not supported", baud);
	return i->second;
}
