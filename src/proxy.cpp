#include "proxy.h"
#include "config.h"
#include "throw.h"
#include "log.h"

static const char SIG_CMD[] = "RM1";
static const char CMP_CMD[] = "RM3";
static const char ALC_CMD[] = "RM4";
static const char PWR_CMD[] = "RM5";
static const char SWR_CMD[] = "RM6";
static const char IDD_CMD[] = "RM7";

Proxy::Proxy(std::vector<uint8_t> &portSendq_, std::vector<uint8_t> &ptySendq_, TimerFd &timer_) : portSendq(portSendq_), ptySendq(ptySendq_), timer(timer_)
{
	timer.start(config::POLL_INTERVAL);
}

void Proxy::feedFromPort(std::vector<uint8_t> &portRecvq)
{
	const std::vector<std::string> commands = feedRecvqToReader(portRecvq, portReader);

	for(std::vector<std::string>::const_iterator i = commands.begin(); i != commands.end(); ++i) {
		if(state == STATE_PROXYING || !handleMeterResponse(*i)) {
			logd("Proxying response from radio: %s", i->c_str());
			std::copy(i->begin(), i->end(), std::back_inserter(ptySendq));
		}
		else {
			logd("Response from radio consumed by us: %s", i->c_str());
		}
	}
}

void Proxy::feedFromPty(std::vector<uint8_t> &ptyRecvq)
{
	if(state != STATE_PROXYING) {
		/* Ignore it for now, it will wait in the recvq */
		return;
	}

	const std::vector<std::string> commands = feedRecvqToReader(ptyRecvq, ptyReader);
	for(std::vector<std::string>::const_iterator i = commands.begin(); i != commands.end(); ++i) {
		logd("Proxying command to radio: %s", i->c_str());
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
	if(state == STATE_PROXYING) {
		logd("Poll timer fired, starting polling");
		setState(STATE_READING_IDD);
		return;
	}

	logn("CAT timeout in state %d, going to proxy again", state);
	setState(STATE_PROXYING);
}

std::optional<Meters> Proxy::getMeters()
{
	if(state != STATE_PROXYING || !meters) {
		/* If not in proxying, then meters are still being updated */
		return std::nullopt;
	}

	const Meters m = *meters;
	meters.reset();
	return m;
}

bool Proxy::handleMeterResponse(const std::string &cmd)
{
	logd("About to handle response %s in state %d", cmd.c_str(), state);

	std::optional<uint8_t> raw;

	switch(state) {
		case STATE_PROXYING:
			xthrow("handleMeterResponse() called while proxying");
			break;

		case STATE_READING_IDD:
			raw = readMeterResponse(cmd, IDD_CMD);
			if(!raw) {
				return false;
			}

			if(!*raw) {
				logd("Idd is zero, we're in RX mode");
				setState(STATE_READING_SIG);
			}
			else {
				logd("Idd is not zero, we're in TX mode");
				meters->idd.emplace(*raw);
				setState(STATE_READING_CMP);
			}
			break;

		case STATE_READING_SIG:
			raw = readMeterResponse(cmd, SIG_CMD);
			if(!raw) {
				return false;
			}

			meters->sig.emplace(*raw);
			setState(STATE_PROXYING);
			break;

		case STATE_READING_CMP:
			raw = readMeterResponse(cmd, CMP_CMD);
			if(!raw) {
				return false;
			}

			meters->cmp.emplace(*raw);
			setState(STATE_READING_ALC);
			break;

		case STATE_READING_ALC:
			raw = readMeterResponse(cmd, ALC_CMD);
			if(!raw) {
				return false;
			}

			meters->alc.emplace(*raw);
			setState(STATE_READING_PWR);
			break;

		case STATE_READING_PWR:
			raw = readMeterResponse(cmd, PWR_CMD);
			if(!raw) {
				return false;
			}

			meters->pwr.emplace(*raw);
			setState(STATE_READING_SWR);
			break;

		case STATE_READING_SWR:
			raw = readMeterResponse(cmd, SWR_CMD);
			if(!raw) {
				return false;
			}

			meters->swr.emplace(*raw);
			setState(STATE_PROXYING);
			break;
	}

	return true;
}

std::optional<uint8_t> Proxy::readMeterResponse(const std::string &cmd, const std::string &expectedCmd)
{
	logd("Trying to match response %s to request %s", cmd.c_str(), expectedCmd.c_str());
	xassert(expectedCmd.size() == 3, "Expected command size invalid");

	/* Under assumption that expectedCmd.size() < 7... */
	if(cmd.size() != 7) {
		logd("Wrong length");
		return std::nullopt;
	}

	if(cmd.substr(0, 3) != expectedCmd) {
		logd("No match");
		return std::nullopt;
	}

	const unsigned value = strtol(cmd.substr(3).c_str(), nullptr, 10);
	if(value > 0xff) {
		logd("Bad value");
		/* Something's wrong, pass the command to pty */
		return std::nullopt;
	}

	logd("Match found, meter value is %u", value);
	return value;
}

void Proxy::setState(State newState)
{
	state = newState;
	timer.start((state == STATE_PROXYING) ? config::POLL_INTERVAL : config::CAT_TIMEOUT);

	switch(newState) {
		case STATE_PROXYING:
			break;

		case STATE_READING_IDD:
			meters.emplace();
			addCommandToPort(IDD_CMD);
			break;

		case STATE_READING_SIG:
			addCommandToPort(SIG_CMD);
			break;

		case STATE_READING_CMP:
			addCommandToPort(CMP_CMD);
			break;

		case STATE_READING_ALC:
			addCommandToPort(ALC_CMD);
			break;

		case STATE_READING_PWR:
			addCommandToPort(PWR_CMD);
			break;

		case STATE_READING_SWR:
			addCommandToPort(SWR_CMD);
			break;

		default:
			xthrow("Invalid state %d", state);
	}
}

void Proxy::addCommandToPort(const std::string &cmd)
{
	logd("Adding our command to the send queue: %s;", cmd.c_str());
	std::copy(cmd.begin(), cmd.end(), std::back_inserter(portSendq));
	portSendq.push_back(';');
}
