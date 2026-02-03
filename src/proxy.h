#pragma once

#include <vector>
#include <string>
#include <cinttypes>
#include <optional>
#include "timerfd.h"
#include "meters.h"
#include "catreader.h"

class Proxy {
public:
	Proxy(std::vector<uint8_t> &portSendq, std::vector<uint8_t> &ptySendq, TimerFd &timer);

	/* Current implementation consumes all data from vectors, but it's not a given */
	void feedFromPort(std::vector<uint8_t> &portRecvq);
	void feedFromPty(std::vector<uint8_t> &ptyRecvq);
	void timerFired();

	/* Call after feedFromPort */
	std::optional<Meters> getMeters();

private:
	std::vector<uint8_t> &portSendq;
	std::vector<uint8_t> &ptySendq;
	TimerFd &timer;

	enum State {
		STATE_PROXYING, /* Timer is POLL_INTERVAL, proxying */
		/* From now on, timer is CAT_TIMEOUT */
		STATE_READING_IDD, /* If Idd is 0, read signal, else read cmp, alc, pwr, swr */
		STATE_READING_SIG, /* Idd was 0, reading signal and going to PROXYING */
		STATE_READING_CMP,
		STATE_READING_ALC,
		STATE_READING_PWR,
		STATE_READING_SWR, /* Once this finishes we go to PROXYING */
	};

	State state{STATE_PROXYING};
	std::optional<Meters> meters;
	CatReader portReader;
	CatReader ptyReader;

	std::vector<std::string> feedRecvqToReader(std::vector<uint8_t> &recvq, CatReader &reader);
	bool handleMeterResponse(const std::string &cmd);
	void setState(State newState);
	std::optional<uint8_t> readMeterResponse(const std::string &cmd, const std::string &expectedCmd);
	void addCommandToPort(const std::string &cmd);
};
