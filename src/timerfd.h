#pragma once

#include <thread>
#include <cinttypes>
#include "socketpair.h"
#include "fd.h"

class TimerFd: public Fd {
public:
	TimerFd(uint32_t interval_ = 0);
	virtual ~TimerFd();

	bool read();

	void start();
	void start(uint32_t interval_);
	void stop();

private:
	SocketPair sp;
	volatile uint32_t interval;
	std::thread thread;

	void sendCommand(uint8_t cmd);
};
