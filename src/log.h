#pragma once

#include <cstdio>

// TODO make this configurable + toggle debug with USR1/USR2

/* Very simple logging for now */
#define logx(...)                                       \
	do {                                                \
		fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
		fprintf(stderr, __VA_ARGS__);                   \
		fprintf(stderr, "\n");                          \
	} while(0)

#define logn(...) logx(__VA_ARGS__)
#define logd(...) (void) 0
// #define logd(...) logx(__VA_ARGS__)
