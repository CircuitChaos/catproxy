#include "model_ft891.h"
#include "interpolator.h"
#include "throw.h"
#include "util.h"
#include "log.h"

static const unsigned METER_SIG = 1;
static const unsigned METER_CMP = 3;
static const unsigned METER_ALC = 4;
static const unsigned METER_PWR = 5;
static const unsigned METER_SWR = 6;
static const unsigned METER_IDD = 7;

std::string ModelFT891::getName()
{
	return "FT-891";
}

ModelMeterList ModelFT891::getMeterList()
{
	return {"Sig", "Cmp", "Alc", "Pwr", "Swr", "Idd"};
}

unsigned ModelFT891::getMaxCookedSize() const
{
	return 7;
}

unsigned ModelFT891::getMeterCount() const
{
	return 6;
}

void ModelFT891::reset()
{
	state = STATE_OFF;
}

std::string ModelFT891::start()
{
	xassert(state == STATE_OFF, "start() called in wrong state (%d)", state);
	state = STATE_IDD;
	createDefaultMeters();
	return createCommand(METER_IDD);
}

std::optional<std::string> ModelFT891::handleResponse(const std::string &rsp)
{
	unsigned meter;
	uint8_t raw;
	if(!parseResponse(rsp, meter, raw)) {
		return std::nullopt;
	}

	unsigned expMeter;
	switch(state) {
		case STATE_IDD:
			expMeter = METER_IDD;
			break;

		case STATE_SIG:
			expMeter = METER_SIG;
			break;

		case STATE_CMP:
			expMeter = METER_CMP;
			break;

		case STATE_ALC:
			expMeter = METER_ALC;
			break;

		case STATE_PWR:
			expMeter = METER_PWR;
			break;

		case STATE_SWR:
			expMeter = METER_SWR;
			break;

		default:
			xthrow("handleResponse() called in wrong state (%d)", state);
			break;
	}

	if(meter != expMeter) {
		logn("Warning: Got valid response %s, but for a meter that we're not expecting now (we wanted %u, but got %u).", rsp.c_str(), expMeter, meter);
		logn("If you're using a program that's also reading radio meters, CAT Proxy will eventually get confused.");
		return std::nullopt;
	}

	switch(state) {
		case STATE_IDD:
			if(raw == 0) {
				logd("Idd is 0, we're in RX");
				state = STATE_SIG;
				return createCommand(METER_SIG);
			}

			logd("Idd is nonzero, we're in TX");
			addMeter(meter, raw);
			state = STATE_CMP;
			return createCommand(METER_CMP);

		case STATE_SIG:
			addMeter(meter, raw);
			state = STATE_DONE;
			return "";

		case STATE_CMP:
			addMeter(meter, raw);
			state = STATE_ALC;
			return createCommand(METER_ALC);

		case STATE_ALC:
			addMeter(meter, raw);
			state = STATE_PWR;
			return createCommand(METER_PWR);

		case STATE_PWR:
			addMeter(meter, raw);
			state = STATE_SWR;
			return createCommand(METER_SWR);

		case STATE_SWR:
			addMeter(meter, raw);
			state = STATE_DONE;
			return "";

		default:
			xthrow("Not expecting state %d here", state);
			break;
	}

	xthrow("Not expecting to be here");
	return std::nullopt;
}

Meters ModelFT891::getMeters()
{
	if(state != STATE_DONE) {
		return Meters();
	}

	logd("Returning meters");
	Meters m = meters;
	meters.clear();
	state = STATE_OFF;
	return m;
}

std::string ModelFT891::createCommand(unsigned meter)
{
	return util::format("RM%u;", meter);
}

bool ModelFT891::parseResponse(const std::string &rsp, unsigned &meter, uint8_t &raw)
{
	logd("Parsing response: %s", rsp.c_str());
	if(rsp.size() != 7) {
		logd("Wrong response size -- it's not for us");
		return false;
	}

	if(rsp.substr(0, 2) != "RM" || rsp[6] != ';') {
		logd("Wrong response format -- it's not for us");
		return false;
	}

	const long rawLong = strtol(rsp.substr(3, 3).c_str(), nullptr, 10);
	xassert(rawLong >= 0 && rawLong <= 0xff, "Meter value out of allowed range");
	meter = rsp[2] - '0';
	raw   = rawLong;
	logd("Response is for meter %u, raw value is %u", meter, raw);
	return true;
}

void ModelFT891::createDefaultMeters()
{
	meters.clear();

	/* Adding meters according to the list from getMeterList(), but
	 * sorted in a more meaningful way.
	 */

	static const std::vector<std::string> list = {"Sig", "Swr", "Pwr", "Cmp", "Alc", "Idd"};
	for(std::vector<std::string>::const_iterator i = list.begin(); i != list.end(); ++i) {
		Meter m;
		m.available = false;
		m.isSwr     = *i == "Swr";
		m.name      = *i;
		m.raw       = 0;
		meters.push_back(m);
	}
}

void ModelFT891::addMeter(unsigned meter, uint8_t raw)
{
	std::string name;
	std::string cooked;

	switch(meter) {
		case METER_SIG:
			name   = "Sig";
			cooked = cookSig(raw);
			break;

		case METER_CMP:
			name   = "Cmp";
			cooked = cookCmp(raw);
			break;

		case METER_ALC:
			name   = "Alc";
			cooked = cookAlc(raw);
			break;

		case METER_PWR:
			name   = "Pwr";
			cooked = cookPwr(raw);
			break;

		case METER_SWR:
			name   = "Swr";
			cooked = cookSwr(raw);
			break;

		case METER_IDD:
			name   = "Idd";
			cooked = cookIdd(raw);
			break;

		default:
			xthrow("Unsupported meter %u", meter);
	}

	for(Meters::iterator i = meters.begin(); i != meters.end(); ++i) {
		if(i->name == name) {
			i->available = true;
			i->raw       = raw;
			i->cooked    = cooked;
			return;
		}
	}

	xthrow("Meter %u (%s) not found on meter list (%zu entries)", meter, name.c_str(), meters.size());
}

std::string ModelFT891::cookSig(uint8_t raw)
{
	// Based on FT891_STR_CAL: https://github.com/Hamlib/Hamlib/blob/master/rigs/yaesu/ft891.h#L88
	static const std::vector<InterpolatorEntry> cal = {
	    {0, -54, "S0"},
	    {12, -48, "S1"},
	    {27, -42, "S2"},
	    {40, -36, "S3"},
	    {55, -30, "S4"},
	    {65, -24, "S5"},
	    {80, -18, "S6"},
	    {95, -12, "S7"},
	    {112, -6, "S8"},
	    {130, 0, "S9"},
	    {150, 10, "S9+10"},
	    {172, 20, "S9+20"},
	    {190, 30, "S9+30"},
	    {220, 40, "S9+40"},
	    {240, 50, "S9+50"},
	    {255, 60, "S9+60"},
	};

	const std::pair<double, std::string> result(interpolate(raw, cal));
	return result.second;
}

std::string ModelFT891::cookCmp(uint8_t raw)
{
	/* Done by counting pixels:
	 *
	 * 0  5  10  15  20  25  30
	 * 0 22  37  49  61  73  87
	 */

	static const std::vector<InterpolatorEntry> cal = {
	    {0, 0.0, ""},
	    {22 * 255 / 87, 5.0, ""},
	    {37 * 255 / 87, 10.0, ""},
	    {49 * 255 / 87, 15.0, ""},
	    {61 * 255 / 87, 20.0, ""},
	    {73 * 255 / 87, 25.0, ""},
	    {87 * 255 / 87, 30.0, ""},
	};

	return util::format("%.1f dB", interpolate(raw, cal).first);
}

std::string ModelFT891::cookAlc(uint8_t raw)
{
	static const std::vector<InterpolatorEntry> cal = {
	    {0, 0, ""},
	    {157, 100, ""},
	    {255, 200, ""},
	};

	return util::format("%.0f%%", interpolate(raw, cal).first);
}

std::string ModelFT891::cookPwr(uint8_t raw)
{
	// Based on FT891_RFPOWER_METER_CAL: https://github.com/Hamlib/Hamlib/blob/master/rigs/yaesu/ft891.h#L73
	static const std::vector<InterpolatorEntry> cal = {
	    {0, 0.0, ""},
	    {10, 0.8, ""},
	    {50, 8.0, ""},
	    {100, 26.0, ""},
	    {150, 54.0, ""},
	    {200, 92.0, ""},
	    {250, 140.0, ""},
	};

	return util::format("%.1f W", interpolate(raw, cal).first);
}

std::string ModelFT891::cookSwr(uint8_t raw)
{
	/* Done by counting pixels:
	 * 1 1.5 2  3  inf
	 * 0 19  37 52 97
	 */

	static const std::vector<InterpolatorEntry> cal = {
	    {0, 1.0, ""},
	    {19 * 255 / 97, 1.5, ""},
	    {37 * 255 / 97, 2.0, ""},
	    {52 * 255 / 97, 3.0, ""},
	};

	if(raw > 52 * 255 / 97) {
		return ">3!!!";
	}

	return util::format("%.2f", interpolate(raw, cal).first);
}

std::string ModelFT891::cookIdd(uint8_t raw)
{
	static const std::vector<InterpolatorEntry> cal = {
	    {0, 0, ""},
	    {255, 30, ""},
	};

	return util::format("%.1f A", interpolate(raw, cal).first);
}

uint32_t ModelFT891::decodeFreq(const std::string &rsp)
{
	if(rsp.size() != 12 || rsp.substr(0, 2) != "FA" || rsp[11] != ';') {
		return 0;
	}

	return strtoul(rsp.substr(2, 9).c_str(), nullptr, 10);
}
