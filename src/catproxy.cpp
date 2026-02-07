#include <stdexcept>
#include <algorithm>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cerrno>
#include <csignal>
#include "throw.h"
#include "config.h"
#include "confkeys.h"
#include "cli.h"
#include "portfd.h"
#include "ptyfd.h"
#include "signalfd.h"
#include "timerfd.h"
#include "proxy.h"
#include "log.h"
#include "meters.h"
#include "util.h"
#include "model.h"
#include "output.h"

static const size_t MAX_QUEUE_SIZE = 8192; /* More than enough */
static const char DEFAULT_CONFIG_FILE[] = "~/.catproxy.conf";

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
	logd("Read %zu bytes from fd %d, recvq now has %zu bytes", (size_t) rs, fd, buf.size());
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
		logd("Written all %zu bytes to fd %d, clearing sendq", (size_t) rs, fd);
		buf.clear();
		return;
	}

	buf.erase(buf.begin(), buf.begin() + rs);
	logd("Written %zu bytes to fd %d, %zu bytes still waiting in sendq", (size_t) rs, fd, buf.size());
}

static void Main(int argc, char *const argv[])
{
	Cli cli(argc, argv);

	if(cli.getExit()) {
		/* -h provided -- clean exit */
		return;
	}

	if(cli.getList()) {
		const ModelList ml = getModelList();
		for(ModelList::const_iterator i = ml.begin(); i != ml.end(); ++i) {
			printf("Model name: %s\n", i->first.c_str());
			for(ModelMeterList::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
				printf("  Meter name: %s\n", j->c_str());
			}
		}

		printf("\n");
		const OutputList ol = getOutputList();
		for(OutputList::const_iterator i = ol.begin(); i != ol.end(); ++i) {
			printf("Output name: %s\n", i->c_str());
		}

		printf("\n");
		return;
	}

	if(cli.getDebug()) {
		loggerSetDebug(true);
	}

	std::string configFile = cli.getConfigFile();
	if(configFile.empty()) {
		configFile = util::applyUserHome(DEFAULT_CONFIG_FILE);
		logd("Using default config file: %s", configFile.c_str());
	}
	else {
		logd("Using custom config file: %s", configFile.c_str());
	}

	Config conf(configFile);
	PortFd port(conf.getString(config::PORT_DEVICE), conf.getInt(config::PORT_BAUDRATE));
	PtyFd pty(util::applyUserHome(conf.getString(config::PTY_SYMLINK)));
	SignalFd sig;
	TimerFd timer;

	std::unique_ptr<Model> model(createModel(conf.getString(config::METERS_RADIO_MODEL)));
	xassert(model, "Unsupported radio model %s", conf.getString(config::METERS_RADIO_MODEL).c_str());

	std::unique_ptr<Output> output(createOutput(conf.getString(config::OUTPUT_TYPE), conf, model->getMeterCount(), model->getMaxCookedSize()));
	xassert(output, "Unsupported output type %s", conf.getString(config::OUTPUT_TYPE).c_str());

	std::vector<uint8_t> portSendq;
	std::vector<uint8_t> portRecvq;
	std::vector<uint8_t> ptySendq;
	std::vector<uint8_t> ptyRecvq;

	Proxy proxy(conf, *model, portSendq, ptySendq, timer);

	/* This is just to help */
	static const size_t portIdx  = 0;
	static const size_t ptyIdx   = 1;
	static const size_t sigIdx   = 2;
	static const size_t timerIdx = 3;
	ssize_t outputIdx            = -1;

	std::vector<pollfd> fds;
	fds.resize(4);

	fds[portIdx].fd      = port;
	fds[ptyIdx].fd       = pty;
	fds[sigIdx].fd       = sig;
	fds[timerIdx].fd     = timer;
	fds[portIdx].events  = POLLIN;
	fds[ptyIdx].events   = POLLIN;
	fds[sigIdx].events   = POLLIN;
	fds[timerIdx].events = POLLIN;

	logd("Port fd is %d, pty fd is %d, sig fd is %d, timer fd is %d", (int) port, (int) pty, (int) sig, (int) timer);

	if(output->getFd() >= 0) {
		logd("Output has fd and it's %d", output->getFd());
		pollfd outputfd;
		outputfd.fd     = output->getFd();
		outputfd.events = POLLIN;
		fds.push_back(outputfd);
		outputIdx = 4;
	}

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

		if(outputIdx >= 0) {
			fds[outputIdx].revents = 0;
		}

		const int rs = poll(&fds[0], fds.size(), -1);
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

			if(signo == SIGUSR1) {
				loggerSetDebug(true);
				logd("Caught signal SIGUSR1, enabling debug output");
			}
			else if(signo == SIGUSR2) {
				logd("Caught signal SIGUSR2, disabling debug output");
				loggerSetDebug(false);
			}
			else {
				logn("Caught signal %d (%s), terminating", signo, strsignal(signo));
				break;
			}
		}

		if(fds[ptyIdx].revents & POLLIN) {
			readVect(pty, ptyRecvq);
			xassert(ptyRecvq.size() < MAX_QUEUE_SIZE, "Allowed PTY recvq size exceeded");
		}

		if(!ptyRecvq.empty()) {
			proxy.feedFromPty(ptyRecvq);
		}

		if(fds[portIdx].revents & POLLIN) {
			readVect(port, portRecvq);
			xassert(portRecvq.size() < MAX_QUEUE_SIZE, "Allowed port recvq size exceeded");
		}

		if(!portRecvq.empty()) {
			proxy.feedFromPort(portRecvq);
		}

		xassert(ptySendq.size() < MAX_QUEUE_SIZE, "Allowed PTY sendq size exceeded");
		xassert(portSendq.size() < MAX_QUEUE_SIZE, "Allowed port sendq size exceeded");

		if((fds[timerIdx].revents & POLLIN) && timer.read()) {
			proxy.timerFired();
		}

		const Meters meters = model->getMeters();
		if(!meters.empty()) {
			output->update(meters);
		}

		if(outputIdx >= 0 && (fds[outputIdx].revents & POLLIN) && !output->read()) {
			logn("Terminating due to output request (window closed, etc.).");
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
