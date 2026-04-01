#include <inttypes.h>
#include <sys/select.h>
#include <unistd.h>
#include "timerfd.h"
#include "throw.h"

/* Timer protocol:
 * - (re)start timer: write '1'
 * - stop timer: write '0'
 * - if timer elapsed, reads 'x'
 * - EOF: terminate thread
 */

static void threadFunc(SocketPair *sp, std::atomic<uint32_t> *interval)
{
	useconds_t usecNeeded(0);
	bool timerEnabled(false);

	for(;;) {
		fd_set rfd;
		FD_ZERO(&rfd);
		FD_SET(sp->get1(), &rfd);

		timeval tv;
		tv.tv_sec  = 0;
		tv.tv_usec = usecNeeded;
		const int rs(select(sp->get1() + 1, &rfd, nullptr, nullptr, timerEnabled ? &tv : nullptr));

		if(rs == 0) {
			/* Timed out */
			if(!timerEnabled) {
				/* Error, timed out but timeout not expected now */
				break;
			}

			/* Signal timeout and stop timer */
			const uint8_t ch('x');
			if(write(sp->get1(), &ch, 1) != 1) {
				break;
			}

			timerEnabled = false;
		}
		else if(rs == 1) {
			/* Activity on socket */
			uint8_t ch;
			const ssize_t rrs(read(sp->get1(), &ch, 1));
			if(rrs != 1) {
				/* Error or EOF, in any case, break */
				break;
			}

			if(ch == '0') {
				/* Stop timer if it's active */
				timerEnabled = false;
			}
			else if(ch == '1') {
				/* Start or restart timer */
				usecNeeded   = *interval * 1000;
				timerEnabled = true;
			}
			else {
				/* Invalid char received, break */
				break;
			}
		}
		else {
			/* Something else returned by select, unsupported */
			break;
		}
	}

	sp->close1();
}

TimerFd::TimerFd(uint32_t interval_)
    : interval(interval_),
      thread(std::thread(threadFunc, &sp, &interval))
{
	setFd(sp.get0(), true);
}

TimerFd::~TimerFd()
{
	sp.close0();
	thread.join();
}

bool TimerFd::read()
{
	uint8_t ch;
	const ssize_t rs(::read(getFd(), &ch, 1));
	xassert(rs == 1, "read() failed: %zd, %m", rs);
	xassert(ch == 'x', "Invalid character read from socket: 0x%02x", ch);
	return true;
}

void TimerFd::start()
{
	sendCommand('1');
}

void TimerFd::start(uint32_t interval_)
{
	interval = interval_;
	sendCommand('1');
}

void TimerFd::stop()
{
	sendCommand('0');
}

void TimerFd::sendCommand(uint8_t cmd)
{
	const ssize_t rs(write(getFd(), &cmd, 1));
	xassert(rs == 1, "write() failed: %zd, %m", rs);
}
