#include "named_pipe.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <iostream>
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
		int ret = mkfifo(filename.c_str(), 0666);
		if(ret < 0)
		{
			m_good = false;
			throw std::runtime_error("Could not open named pipe " + filename + ".");
		}
		m_good = true;
    }
	~Impl()
	{
		std::cerr << "destructor for reader->impl." << std::endl;
		if(m_good)
		{
			std::cerr << "destructor for reader->impl calling unlink pipe: " << filename << std::endl;
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
			throw std::runtime_error("Could not open file for reading: " + filename + ".");
			return false;
		}
        read(fd, tmpbuf, nbytes);
        close(fd);

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
			throw std::runtime_error("Could not open named pipe " + filename + ".");
		}
		std::cerr << "mkfifo succeeded: " << filename << std::endl;
		m_good = true;
    }
	~Impl()
	{
		std::cerr << "destructor for writer->impl." << std::endl;
	}
	bool good() const {return m_good;}

	bool write_buf(void *buf, const std::size_t nbytes) const
	{
		std::cerr << "writing buf inside writer::impl:" << std::endl;
		uint8_t* tmpbuf = static_cast<uint8_t*>(buf);
		std::cerr << "cast to tmpbuf." << std::endl;
		std::cerr << "opening file for writing:" << filename << std::endl;
		// Note: opening the file and writing to it to act as lock for now.
		int fd = open(filename.c_str(), O_WRONLY);
		std::cerr << "opened file for writing:" << filename << " and fd:" << fd << std::endl;
		if(fd < 0)
		{
			throw std::runtime_error("Could not open file for writing: " + filename + ".");
			return false;
		}
		std::cerr << "attempting write tmpbuf:" << std::endl;
		std::cerr.flush();
        write(fd, tmpbuf, nbytes);
		std::cerr << "wrote tmpbuf!" << std::endl;
        close(fd);
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
