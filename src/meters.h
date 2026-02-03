#pragma once

#include <optional>
#include <string>
#include <cstdint>
#include <cinttypes>

namespace meters {

/* All toHuman return up to 7 chars */

class Meter {
public:
	Meter(uint8_t raw_) : raw(raw_) {}
	virtual ~Meter() {}

	uint8_t getRaw() const;
	virtual std::string toHuman() = 0;

private:
	const uint8_t raw;
};

class Signal : public Meter {
public:
	Signal(uint8_t raw_) : Meter(raw_) {}
	virtual ~Signal() {}

	virtual std::string toHuman();
};

class Compressor : public Meter {
public:
	Compressor(uint8_t raw_) : Meter(raw_) {}
	virtual ~Compressor() {}

	virtual std::string toHuman();
};

class Alc : public Meter {
public:
	Alc(uint8_t raw_) : Meter(raw_) {}
	virtual ~Alc() {}

	virtual std::string toHuman();
};

class Power : public Meter {
public:
	Power(uint8_t raw_) : Meter(raw_) {}
	virtual ~Power() {}

	virtual std::string toHuman();
};

class Swr : public Meter {
public:
	Swr(uint8_t raw_) : Meter(raw_) {}
	virtual ~Swr() {}

	virtual std::string toHuman();
};

class Idd : public Meter {
public:
	Idd(uint8_t raw_) : Meter(raw_) {}
	virtual ~Idd() {}

	virtual std::string toHuman();
};

} // namespace meters

struct Meters {
	Meters() {}
	Meters(const Meters &meters);
	Meters &operator=(const Meters &meters);

	std::optional<meters::Signal> sig;
	std::optional<meters::Compressor> cmp;
	std::optional<meters::Alc> alc;
	std::optional<meters::Power> pwr;
	std::optional<meters::Swr> swr;
	std::optional<meters::Idd> idd;
};
