#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>

#include<iostream>
#include<streambuf>
#include<vector>
#include<string>

#include<stdexcept>
#include<system_error>

#include "vidio_internal.hpp"
#include "pstream.h"

namespace vidio
{
	
	
#ifndef USE_PSTREAMS
namespace posix
{

	

class fake_streambuf: public std::streambuf
{
public:
	fake_streambuf(const std::string& cmd)
	{
		std::cerr << "Now executing:\n>" << cmd << std::endl;
	}
};

class process_streambuf_base: public std::streambuf
{
protected:
	FILE* pipefile;
public:
	enum ReadWriteType
	{
		READ_ONLY=0,
		WRITE_ONLY
	};
	process_streambuf_base(const std::string& cmd,ReadWriteType rwflag)
	{
		static const char* typestring[2]={"r","w"};
		std::cerr << "Attempting to run Command:\n>" << cmd << std::endl;
		pipefile=popen(cmd.c_str(),typestring[(int)rwflag]);
		if(!pipefile)
		{
			throw std::system_error(errno,std::system_category());
		}
	}
	virtual ~process_streambuf_base()
	{
		std::cerr << "Close";
		if(pipefile)
		{
			int exitcode=pclose(pipefile); //log if return code non-0? Can't throw from a destructor!
			std::cerr<< "child ffmpeg process exited with non-zero return code: " << exitcode << "\n";
		}
	}
};

class process_readstreambuf: public process_streambuf_base
{
public:
	process_readstreambuf(const std::string& cmd):
		process_streambuf_base(cmd,READ_ONLY)
	{}
protected:
	virtual int underflow()
	{
		int i=fgetc(pipefile);
		std::cerr << "Read char: " << (char)i << std::endl;
		return i;
	}
	virtual std::streamsize xsgetn(char* bufout, std::streamsize n)
	{
		return fread(bufout,n,1,pipefile);
	}
};

class process_writestreambuf: public process_streambuf_base
{
public:
	process_writestreambuf(const std::string& cmd):
		process_streambuf_base(cmd,WRITE_ONLY)
	{}
protected:
	virtual int overflow(int c=EOF)
	{
		return fputc(c,pipefile);
	}
	virtual std::streamsize xsputn(const char* bufin, std::streamsize n)
	{
		return fwrite(bufin,n,1,pipefile);
	}
};

}


#endif


namespace platform
{
	
/*
//public implementation of platform-specific code
std::streambuf* create_process_reader_streambuf(const std::string& cmd)
{
	//return new posix::process_readstreambuf(cmd);
	return new pstreambuf(cmd
	//return new posix::fake_streambuf(cmd);
}

std::streambuf* create_process_writer_streambuf(const std::string& cmd)
{
	return new posix::process_writestreambuf(cmd);
}*/

std::shared_ptr<std::istream> create_process_reader_stream(const std::string& cmd)
{
	return std::shared_ptr<std::istream>(new redi::ipstream(cmd));
}
std::shared_ptr<std::ostream> create_process_writer_stream(const std::string& cmd)
{
	return std::shared_ptr<std::ostream>(new redi::opstream(cmd));
}


const std::vector<std::string>& get_default_ffmpeg_search_locations()
{
	static const std::vector<std::string> possible_locations{"ffmpeg","/usr/bin/ffmpeg","/usr/local/bin/ffmpeg"};
	return possible_locations;
}
}

}
