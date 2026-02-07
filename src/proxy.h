#pragma once

#include <vector>
#include <string>
#include <cinttypes>
#include <optional>
#include "timerfd.h"
#include "meters.h"
#include "catreader.h"
#include "config.h"
#include "model.h"

class Proxy {
public:
	Proxy(const Config &conf, Model &model, std::vector<uint8_t> &portSendq, std::vector<uint8_t> &ptySendq, TimerFd &timer);

	/* Current implementation consumes all data from vectors, but it's not a given */
	void feedFromPort(std::vector<uint8_t> &portRecvq);
	void feedFromPty(std::vector<uint8_t> &ptyRecvq);
	void timerFired();

private:
	Model &model;
	const unsigned pollInterval;
	const unsigned catTimeout;
	std::vector<uint8_t> &portSendq;
	std::vector<uint8_t> &ptySendq;
	TimerFd &timer;

	/* If it's false, then timer is POLL_INTERVAL and we're proxying.
	 * If it's true, then timer is CAT_TIMEOUT and we're reading meters.
	 */
	bool reading{false};
	CatReader portReader;
	CatReader ptyReader;

	std::vector<std::string> feedRecvqToReader(std::vector<uint8_t> &recvq, CatReader &reader);
};
