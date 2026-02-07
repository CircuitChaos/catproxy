#include "output.h"
#include "output_x11.h"
#include "output_table.h"

OutputList getOutputList()
{
	return {OutputX11::getName(), OutputTable::getName()};
}

Output *createOutput(const std::string &name, const Config &conf, unsigned meterCount, unsigned maxCookedSize)
{
	if(name == OutputX11::getName()) {
		return new OutputX11(conf, meterCount, maxCookedSize);
	}

	if(name == OutputTable::getName()) {
		return new OutputTable();
	}

	return nullptr;
}
