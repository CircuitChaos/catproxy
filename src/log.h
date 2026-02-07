#pragma once

#include <string>

void loggerLog(bool debug, const char *file, int line, const char *fmt, ...) __attribute__((format(printf, 4, 5)));
void loggerSetDebug(bool debug);

#define logn(...) loggerLog(false, __FILE__, __LINE__, __VA_ARGS__)
#define logd(...) loggerLog(true, __FILE__, __LINE__, __VA_ARGS__)
