#include "interpolator.h"
#include "throw.h"

std::pair<double, std::string> interpolate(uint8_t raw, const std::vector<InterpolatorEntry> &cal)
{
	xassert(raw >= cal[0].raw && raw <= cal[cal.size() - 1].raw, "Meter value out of range");

	std::vector<InterpolatorEntry>::const_iterator prev(cal.begin());
	std::vector<InterpolatorEntry>::const_iterator next;
	for(std::vector<InterpolatorEntry>::const_iterator i(cal.begin()); i != cal.end(); ++i) {
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
