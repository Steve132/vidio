#include "named_pipe.hpp"
#include <fcntl.h> // open, close, read, and write()
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <cerrno> // read error set by mkfifo with errno
#include <cstring> // strerror for errno for mkfifo
#include <sstream> // for printing info for exceptions
using namespace std;

namespace named_pipe
{

class Reader::Impl
{
public:
	bool m_good;
	std::string filename;
	Impl(const std::string& filename_str)
	{
		filename = filename_str;
		// mkfifo(<pathname>,<permission>)
		int ret = mkfifo(filename_str.c_str(), 0666);
		if((ret < 0) && (errno != EEXIST))
		{
			m_good = false;
			throw std::runtime_error("Could not open named pipe " + filename + ".  " + std::strerror(errno));
		}
		m_good = true;
	}
	~Impl()
	{
		if(m_good)
		{
			unlink(filename.c_str());
		}
	}
	bool good() const {return m_good;}

	bool read_buf(void *buf, const std::size_t nbytes) const
	{
		uint8_t* tmpbuf = static_cast<uint8_t*>(buf);
		int fd = open(filename.c_str(), O_RDONLY); // | O_NONBLOCK
		if(fd < 0)
		{
			throw std::runtime_error("Could not open file for reading: " + filename + ".  " + std::strerror(errno));
			return false;
		}
		int nbytes_read = read(fd, tmpbuf, nbytes);
		if(nbytes_read < nbytes)
		{
			close(fd);
			std::stringstream ss;
			ss << "Read did not fill buffer and only read " << nbytes_read << " bytes.   Expected " << nbytes << ".";
			throw std::runtime_error(ss.str());
			return false;
		}
		int ret = close(fd);
		if(ret < 0)
		{
			throw std::runtime_error("Could not close file being read: " + filename + ".  " + std::strerror(errno));
			return false;
		}

		return true;
	}
};

Reader::Reader(const std::string& filename):
	impl(std::make_shared<Reader::Impl>(filename))
{}
Reader::~Reader()
{}

bool Reader::good() const
{
	return impl && impl->good();
}
bool Reader::read_buf(void *buf, const std::size_t nbytes) const
{
	return impl->read_buf(buf, nbytes);
}

class Writer::Impl
{
public:
	bool m_good;
	std::string filename;
	Impl(const std::string& filename_str)
	{
		filename = filename_str;
		// mkfifo(<pathname>,<permission>)
		int ret = mkfifo(filename.c_str(), 0666);
		if(ret < 0)
		{
			m_good = false;
			throw std::runtime_error("Could not open named pipe " + filename + ".  " + std::strerror(errno));
		}
		m_good = true;
	}
	~Impl()
	{
	}
	bool good() const {return m_good;}

	bool write_buf(void *buf, const std::size_t nbytes) const
	{
		uint8_t* tmpbuf = static_cast<uint8_t*>(buf);
		// Note: opening the file and writing to it to act as lock for now.
		int fd = open(filename.c_str(), O_WRONLY);
		if(fd < 0)
		{
			throw std::runtime_error("Could not open file for writing: " + filename + ".  " + std::strerror(errno));
			return false;
		}
		int ret = write(fd, tmpbuf, nbytes);
		if(ret < 0)
		{
			throw std::runtime_error("Could not write tmpbuf.  " + std::string(std::strerror(errno)));
			return false;
		}
		ret = close(fd);
		if(ret < 0)
		{
			throw std::runtime_error("Could not close file being written: " + filename + ".  " + std::strerror(errno));
			return false;
		}
		return true;
	}
};

Writer::Writer(const std::string& filename):
	impl(std::make_shared<Writer::Impl>(filename))
{}
Writer::~Writer()
{}

bool Writer::good() const
{
	return impl && impl->good();
}
bool Writer::write_buf(void *buf, const std::size_t nbytes) const
{
	return impl->write_buf(buf, nbytes);
}

}
