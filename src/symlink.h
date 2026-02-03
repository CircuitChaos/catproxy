#pragma once

#include <string>

class Symlink {
public:
	Symlink(const std::string &target, const std::string &link);
	~Symlink();

private:
	const std::string link;
};
