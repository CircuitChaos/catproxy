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
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);

	const int fd = signalfd(-1, &sigset, 0);
	xassert(fd >= 0, "signalfd() failed: %m");
	setFd(fd);

	sigaddset(&sigset, SIGPIPE);
	xassert(sigprocmask(SIG_BLOCK, &sigset, nullptr) != -1, "sigprocmask() failed: %m");
}

SignalFd::~SignalFd()
{
	sigset_t sigset;
	sigemptyset(&sigset);

	sigaddset(&sigset, SIGTERM);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGUSR1);
	sigaddset(&sigset, SIGUSR2);
	sigaddset(&sigset, SIGPIPE);

	sigprocmask(SIG_UNBLOCK, &sigset, nullptr);
}

int SignalFd::read()
{
	signalfd_siginfo siginfo;
	const ssize_t rs = ::read(getFd(), &siginfo, sizeof(siginfo));

	/* If we were called then signal has to be pending */
	xassert(rs == sizeof(siginfo), "read() from signalfd failed: %zd, %m", rs);
	return siginfo.ssi_signo;
}
