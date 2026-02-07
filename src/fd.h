#pragma once

class Fd {
public:
	virtual ~Fd();
	operator int() const;

	/* For direct usage */
	void reset(int fd, bool noclose_ = false);

protected:
	/* For derived classes */
	/* noclose refers to the new descriptor (fd_) */
	void setFd(int fd_, bool noclose_ = false);
	int getFd();

private:
	int fd{-1};
	bool noclose{false};
};
