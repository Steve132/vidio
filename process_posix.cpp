#include<iostream>
#include<streambuf>
#include<unistd.h>

#include<cstdio>
#include<cerrno>

#include<stdexcept>
#include<system_error>

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
		static const char* typestring[2]={"re","we"};
		pipefile=popen(cmd.c_str(),typestring[(int)rwflag]);
		if(!pipefile)
		{
			throw std::system_error(errno,strerror(errno));
		}
	}
	virtual ~process_streambuf_base()
	{
		int pclose(pipefile);
	}
};

class process_readstreambuf: public process_streambuf_base
{
public:
	process_readstreambuf(const std::string& cmd):
		process_streambuf_base(cmd,READ_ONLY)
	{}
	virtual int underflow()
	{
		return fgetc(pipefile);
	}
	virtual std::streamsize xsgetn(char* bufout, streamsize n)
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
	virtual int overflow(int c=EOF)
	{
		return fputc(pipefile);
	}
	virtual std::streamsize xsputn(const char* bufin, streamsize n)
	{
		return fwrite(bufin,n,1,pipefile);
	}
};