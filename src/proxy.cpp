#include "proxy.h"
#include "throw.h"
#include "log.h"
#include "confkeys.h"

Proxy::Proxy(const Config &conf, Model &model, std::vector<uint8_t> &portSendq, std::vector<uint8_t> &ptySendq, TimerFd &timer)
    : model(model),
      pollInterval(conf.getInt(config::CAT_POLL_INTERVAL)),
      catTimeout(conf.getInt(config::CAT_TIMEOUT)),
      portSendq(portSendq),
      ptySendq(ptySendq),
      timer(timer)
{
	timer.start(pollInterval);
}

void Proxy::feedFromPort(std::vector<uint8_t> &portRecvq)
{
	const std::vector<std::string> responses = feedRecvqToReader(portRecvq, portReader);

	for(std::vector<std::string>::const_iterator i = responses.begin(); i != responses.end(); ++i) {
		bool doProxy = false;

		if(reading) {
			std::optional<std::string> cmd = model.handleResponse(*i);
			if(!cmd) {
				logd("Response %s not consumed by the model, forwarding to the program", i->c_str());
				doProxy = true;
			}
			else {
				if(cmd->empty()) {
					logd("Response %s consumed by the model and no further requests -- full set of meters read", i->c_str());
					timer.start(pollInterval);
					reading = false;
				}
				else {
					logd("Response %s consumed by the model, and model sends new command: %s", i->c_str(), cmd->c_str());
					std::copy(cmd->begin(), cmd->end(), std::back_inserter(portSendq));
				}
			}
		}
		else {
			logd("We're not reading meters now, so unconditionally forwarding response %s to the radio", i->c_str());
			doProxy = true;
		}

		if(doProxy) {
			std::copy(i->begin(), i->end(), std::back_inserter(ptySendq));
		}
	}
}

void Proxy::feedFromPty(std::vector<uint8_t> &ptyRecvq)
{
	if(reading) {
		/* Ignore it for now, it will wait in the recvq */
		return;
	}

	const std::vector<std::string> commands = feedRecvqToReader(ptyRecvq, ptyReader);
	for(std::vector<std::string>::const_iterator i = commands.begin(); i != commands.end(); ++i) {
		logd("Forwarding command %s from program to the radio", i->c_str());
		std::copy(i->begin(), i->end(), std::back_inserter(portSendq));
	}
}

std::vector<std::string> Proxy::feedRecvqToReader(std::vector<uint8_t> &recvq, CatReader &reader)
{
	std::vector<std::string> cmds;

	for(std::vector<uint8_t>::const_iterator i = recvq.begin(); i != recvq.end(); ++i) {
		std::optional<std::string> s(reader.feed(*i));
		if(s) {
			cmds.push_back(*s);
		}
	}

	recvq.clear();
	return cmds;
}

void Proxy::timerFired()
{
	if(!reading) {
		logd("Poll timer fired, starting polling");
		timer.start(catTimeout);
		reading = true;

		const std::string cmd = model.start();
		logd("Forwarding command %s from the model to the radio", cmd.c_str());
		std::copy(cmd.begin(), cmd.end(), std::back_inserter(portSendq));
		return;
	}

	logn("CAT timeout, resetting model and going to proxy again");
	model.reset();
	timer.start(pollInterval);
	reading = false;
}
