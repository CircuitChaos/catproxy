#include <vector>
#include "meters.h"
#include "util.h"
#include "throw.h"

namespace meters {

struct CalEntry {
	uint8_t raw;
	double numeric;
	std::string textual;
};

static std::pair<double, std::string> interpolate(uint8_t raw, const std::vector<CalEntry> &cal)
{
	xassert(raw >= cal[0].raw && raw <= cal[cal.size() - 1].raw, "Meter value out of range");

	std::vector<CalEntry>::const_iterator prev(cal.begin());
	std::vector<CalEntry>::const_iterator next;
	for(std::vector<CalEntry>::const_iterator i(cal.begin()); i != cal.end(); ++i) {
		if(i->raw == raw) {
			return std::pair<double, std::string>(i->numeric, i->textual);
		}

		if(i->raw > raw) {
			next = i;
			break;
		}

		prev = i;
	}

	const uint8_t minRaw(prev->raw);
	const uint8_t maxRaw(next->raw);
	const double minNum(prev->numeric);
	const double maxNum(next->numeric);

	const double rawDelta((double) (raw - minRaw) / (maxRaw - minRaw));
	const double numDiff(maxNum - minNum);
	const double num(numDiff * rawDelta + minNum);

	return std::pair<double, std::string>(num, prev->textual);
}

uint8_t Meter::getRaw() const
{
	return raw;
}

std::string Signal::toHuman()
{
	// Based on FT891_STR_CAL: https://github.com/Hamlib/Hamlib/blob/master/rigs/yaesu/ft891.h#L88
	static const std::vector<CalEntry> cal = {
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

	const std::pair<double, std::string> result(interpolate(getRaw(), cal));
	return util::format("%s", result.second.c_str());
}

std::string Compressor::toHuman()
{
	/* Done by counting pixels:
	 *
	 * 0  5  10  15  20  25  30
	 * 0 22  37  49  61  73  87
	 */

	static const std::vector<CalEntry> cal = {
	    {0, 0.0, ""},
	    {22 * 255 / 87, 5.0, ""},
	    {37 * 255 / 87, 10.0, ""},
	    {49 * 255 / 87, 15.0, ""},
	    {61 * 255 / 87, 20.0, ""},
	    {73 * 255 / 87, 25.0, ""},
	    {87 * 255 / 87, 30.0, ""},
	};

	return util::format("%.1f dB", interpolate(getRaw(), cal).first);
}

std::string Alc::toHuman()
{
	static const std::vector<CalEntry> cal = {
	    {0, 0, ""},
	    {157, 100, ""},
	    {255, 200, ""},
	};

	return util::format("%.0f%%", interpolate(getRaw(), cal).first);
}

std::string Power::toHuman()
{
	// Based on FT891_RFPOWER_METER_CAL: https://github.com/Hamlib/Hamlib/blob/master/rigs/yaesu/ft891.h#L73
	static const std::vector<CalEntry> cal = {
	    {0, 0.0, ""},
	    {10, 0.8, ""},
	    {50, 8.0, ""},
	    {100, 26.0, ""},
	    {150, 54.0, ""},
	    {200, 92.0, ""},
	    {250, 140.0, ""},
	};

	return util::format("%.1f W", interpolate(getRaw(), cal).first);
}

std::string Swr::toHuman()
{
	/* Done by counting pixels:
	 * 1 1.5 2  3  inf
	 * 0 19  37 52 97
	 */

	static const std::vector<CalEntry> cal = {
	    {0, 1.0, ""},
	    {19 * 255 / 97, 1.5, ""},
	    {37 * 255 / 97, 2.0, ""},
	    {52 * 255 / 97, 3.0, ""},
	};

	if(getRaw() > 52 * 255 / 97) {
		return ">3!!!";
	}

	return util::format("%.2f", interpolate(getRaw(), cal).first);
}

std::string Idd::toHuman()
{
	static const std::vector<CalEntry> cal = {
	    {0, 0, ""},
	    {255, 30, ""},
	};

	return util::format("%.1f A", interpolate(getRaw(), cal).first);
}

} // namespace meters

Meters::Meters(const Meters &meters)
{
	if(meters.sig) {
		sig.emplace(*meters.sig);
	}

	if(meters.cmp) {
		cmp.emplace(*meters.cmp);
	}

	if(meters.alc) {
		alc.emplace(*meters.alc);
	}

	if(meters.pwr) {
		pwr.emplace(*meters.pwr);
	}

	if(meters.swr) {
		swr.emplace(*meters.swr);
	}

	if(meters.idd) {
		idd.emplace(*meters.idd);
	}
}

Meters &Meters::operator=(const Meters &meters)
{
	sig.reset();
	cmp.reset();
	alc.reset();
	pwr.reset();
	swr.reset();
	idd.reset();

	if(meters.sig) {
		sig.emplace(*meters.sig);
	}

	if(meters.cmp) {
		cmp.emplace(*meters.cmp);
	}

	if(meters.alc) {
		alc.emplace(*meters.alc);
	}

	if(meters.pwr) {
		pwr.emplace(*meters.pwr);
	}

	if(meters.swr) {
		swr.emplace(*meters.swr);
	}

	if(meters.idd) {
		idd.emplace(*meters.idd);
	}

	return *this;
}
