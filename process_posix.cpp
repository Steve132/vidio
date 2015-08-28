#ifndef PIPED_PROCESS_CPP
#define PIPED_PROCESS_CPP

#include<iostream>

class child_process_streambuf: public std::streambuf
{
public:
	virtual int underflow() 
	{
	}
	virtual std::streamsize xsgetn(char* bufout,std::streamsize n)
	{
		
	}
	virtual std::streamsize xsputn(const char* bufin,std::streamsize n)
	{
		
	}
};







#endif