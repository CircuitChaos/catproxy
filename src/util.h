#pragma once

#include <string>

namespace util {

std::string format(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
std::string applyUserHome(const std::string &path);
std::string toLower(const std::string &s);

} // namespace util
