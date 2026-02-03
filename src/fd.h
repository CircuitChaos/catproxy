#pragma once

class Fd {
public:
	virtual ~Fd();
	operator int() const;

protected:
	/* noclose refers to the new descriptor (fd_) */
	void setFd(int fd_, bool noclose_ = false);
	int getFd();

private:
	int fd{-1};
	bool noclose{false};
};
