#include <cstdio>
#include "output_table.h"

void OutputTable::update(const Meters &meters)
{
	printf("meters_start\n");
	for(std::vector<Meter>::const_iterator i = meters.begin(); i != meters.end(); ++i) {
		printf("meter\t%s\t", i->name.c_str());
		if(!i->available) {
			printf("0\n");
		}
		else {
			printf("1\t%u\t%s\n", i->raw, i->cooked.c_str());
		}
	}
	printf("meters_end\n");
}

std::string OutputTable::getName()
{
	return "table";
}
