#include <stdexcept>
#include <algorithm>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cerrno>
#include "throw.h"
#include "config.h"
#include "portfd.h"
#include "ptyfd.h"
#include "signalfd.h"
#include "timerfd.h"
#include "proxy.h"
#include "log.h"
#include "meters.h"
#include "uifd.h"

/* Appends data read from fd to buf */
static void readVect(int fd, std::vector<uint8_t> &buf)
{
	uint8_t cbuf[4096];
	const ssize_t rs = read(fd, cbuf, sizeof(cbuf));
	xassert(rs != 0, "read(): EOF, device disconnected?");
	if(rs < 0) {
		if(errno == EINTR) {
			return;
		}

		xthrow("read() failed: %m");
	}

	xassert((size_t) rs <= sizeof(cbuf), "read(): rs %zd > buffer size %zu", rs, sizeof(cbuf));
	std::copy(cbuf, cbuf + (size_t) rs, std::back_inserter(buf));
}

/* Writes data from vector, erasing data as it's written */
static void writeVect(int fd, std::vector<uint8_t> &buf)
{
	xassert(!buf.empty(), "writeVect() called on empty vector");
	const ssize_t rs = write(fd, &buf[0], buf.size());
	xassert(rs != 0, "write() returned 0, this shouldn't happen");
	if(rs < 0) {
		if(errno == EINTR) {
			return;
		}

		xthrow("write() failed: %m");
	}

	xassert((size_t) rs <= buf.size(), "write(): rs %zd > buffer size %zu", rs, buf.size());

	if((size_t) rs == buf.size()) {
		/* Typical case, might be faster than erasing range */
		buf.clear();
		return;
	}

	buf.erase(buf.begin(), buf.begin() + rs);
}

static void Main(int argc, char *const /* argv */[])
{
	xassert(argc == 1, "This program doesn't take any arguments");

	PortFd port(config::RADIO_PORT, config::RADIO_BAUD);
	PtyFd pty(config::SYMLINK_PATH);
	SignalFd sig;
	TimerFd timer;
	UiFd ui;

	// TODO add some limits on the queues
	std::vector<uint8_t> portSendq;
	std::vector<uint8_t> portRecvq;
	std::vector<uint8_t> ptySendq;
	std::vector<uint8_t> ptyRecvq;

	/* There are two modes:
	 * - readingMeters = false
	 *   - all data received on pty is sent to port
	 *   - all data received on port is sent to pty
	 *   - pollTimer is started
	 *
	 * Once pollTimer fires, we finish reading last CAT command
	 * from pty and CAT response from port (if we're in the middle
	 * of it) and we go to readingMeters = true
	 *
	 * - readingMeters = true
	 *   - data is not read from pty
	 *   - meter reader performs several steps, starting catTimer
	 *     between them (to catch CAT timeout)
	 *   - if any data is received that's not meant for the meter
	 *     reader, it's sent to pty
	 *   - if meter reader doesn't get requested data within catTimer
	 *     timeout, cat timeout is signalled and reading fails
	 */
	Proxy proxy(portSendq, ptySendq, timer);

	/* This is just to help */
	static const size_t portIdx  = 0;
	static const size_t ptyIdx   = 1;
	static const size_t sigIdx   = 2;
	static const size_t timerIdx = 3;
	static const size_t uiIdx    = 4;

	// UiFd ui;

	struct pollfd fds[5];
	fds[portIdx].fd  = port;
	fds[ptyIdx].fd   = pty;
	fds[sigIdx].fd   = sig;
	fds[timerIdx].fd = timer;
	fds[uiIdx].fd    = ui;

	/* Things that don't change */
	fds[portIdx].events  = POLLIN;
	fds[ptyIdx].events   = POLLIN;
	fds[sigIdx].events   = POLLIN;
	fds[timerIdx].events = POLLIN;
	fds[uiIdx].events    = POLLIN;

	for(;;) {
		if(!portSendq.empty()) {
			fds[portIdx].events |= POLLOUT;
		}
		else {
			fds[portIdx].events &= ~POLLOUT;
		}

		if(!ptySendq.empty()) {
			fds[ptyIdx].events |= POLLOUT;
		}
		else {
			fds[ptyIdx].events &= ~POLLOUT;
		}

		fds[portIdx].revents  = 0;
		fds[ptyIdx].revents   = 0;
		fds[sigIdx].revents   = 0;
		fds[timerIdx].revents = 0;
		fds[uiIdx].revents    = 0;

		const int rs = poll(fds, sizeof(fds) / sizeof(*fds), -1);
		if(rs == -1) {
			if(errno == EINTR) {
				continue;
			}

			xthrow("poll() failed: %m");
		}

		if(fds[portIdx].revents & POLLOUT) {
			writeVect(port, portSendq);
		}

		if(fds[ptyIdx].revents & POLLOUT) {
			writeVect(pty, ptySendq);
		}

		if(fds[sigIdx].revents & POLLIN) {
			const int signo = sig.read();
			logn("Caught signal %d (%s), terminating", signo, strsignal(signo));
			break;
		}

		if(fds[ptyIdx].revents & POLLIN) {
			readVect(pty, ptyRecvq);
		}

		if(!ptyRecvq.empty()) {
			proxy.feedFromPty(ptyRecvq);
		}

		if(fds[portIdx].revents & POLLIN) {
			readVect(port, portRecvq);
		}

		if(!portRecvq.empty()) {
			proxy.feedFromPort(portRecvq);
		}

		if((fds[timerIdx].revents & POLLIN) && timer.read()) {
			proxy.timerFired();
		}

		const std::optional<Meters> meters = proxy.getMeters();
		if(meters) {
			ui.update(*meters);
		}

		if((fds[uiIdx].revents & POLLIN) && !ui.read()) {
			logn("UI window closed, terminating");
			break;
		}
	}
}

int main(int ac, char *const av[])
{
	try {
		Main(ac, av);
	}
	catch(const std::runtime_error &e) {
		fprintf(stderr, "Fatal error: %s\n", e.what());
		return EXIT_FAILURE;
	}

	/* NOTREACHED */
	return EXIT_SUCCESS;
}
