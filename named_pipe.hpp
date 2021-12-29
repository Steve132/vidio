#ifndef NAMEDPIPE_HPP
#define NAMEDPIPE_HPP
#include <memory>
#include <string>

namespace named_pipe
{

class Reader
{
protected:
	class Impl;
	std::shared_ptr<Impl> impl;
public:
	Reader(const std::string& filename);
	bool good() const;
	
	bool read_buf(void *buf, const std::size_t nbytes) const;
	operator bool() const
	{
		return good();
	}

	~Reader();
};

class Writer
{
protected:
	class Impl;
	std::shared_ptr<Impl> impl;
public:
	Writer(const std::string& filename);
	bool good() const;
	
	bool write_buf(void *buf, const std::size_t nbytes) const;
	operator bool() const
	{
		return good();
	}

	~Writer();
};

}

#endif
