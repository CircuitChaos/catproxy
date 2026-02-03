#pragma once

#include <memory>
#include <string>
#include "symlink.h"
#include "fd.h"

class PtyFd: public Fd {
public:
	PtyFd(const std::string &symlinkPath);
	virtual ~PtyFd();

private:
	std::unique_ptr<Symlink> symlink;
};
