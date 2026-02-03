#include <sys/signalfd.h>
#include <signal.h>
#include "signalfd.h"
#include "throw.h"

SignalFd::SignalFd()
{
	sigset_t sigset;
	sigemptyset(&sigset);

	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGINT);

	const int fd = signalfd(-1, &sigset, 0);
	xassert(fd >= 0, "signalfd() failed: %m");
	setFd(fd);
}

SignalFd::~SignalFd()
{
}

int SignalFd::read()
{
	signalfd_siginfo siginfo;
	const ssize_t rs = ::read(getFd(), &siginfo, sizeof(siginfo));

	/* If we were called then signal has to be pending */
	xassert(rs == sizeof(siginfo), "read() from signalfd failed: %zd, %m", rs);
	return siginfo.ssi_signo;
}
