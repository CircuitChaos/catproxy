#include "output.h"
#include "config.h"

class OutputTable : public Output {
public:
	virtual ~OutputTable() {}
	virtual void update(const Meters &meters);
	static std::string getName();
};
