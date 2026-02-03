#include <unistd.h>
#include "symlink.h"
#include "throw.h"
#include "log.h"

Symlink::Symlink(const std::string &target, const std::string &link_) : link(link_)
{
	unlink(link.c_str());
	xassert(symlink(target.c_str(), link.c_str()) == 0, "symlink() failed: %m");
	logd("Created symlink %s pointing to %s", link.c_str(), target.c_str());
}

Symlink::~Symlink()
{
	unlink(link.c_str());
}
