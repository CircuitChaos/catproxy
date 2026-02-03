#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include "throw.h"
#include "ptyfd.h"
#include "log.h"

PtyFd::PtyFd(const std::string &symlinkPath)
{
	const int fd = open("/dev/ptmx", O_RDWR);
	xassert(fd >= 0, "Cannot open /dev/ptmx: %m");
	setFd(fd);

	xassert(!grantpt(fd), "grantpt() failed: %m");
	xassert(!unlockpt(fd), "unlockpt() failed: %m");

	char cslavename[1024];
	const int err = ptsname_r(fd, cslavename, sizeof(cslavename));
	xassert(!err, "slavename() failed: %s", strerror(err));
	logd("PTY created, slave name: %s", cslavename);

	symlink.reset(new Symlink(cslavename, symlinkPath));
}

PtyFd::~PtyFd()
{
}
