#ifndef SUBPROCESS_HPP
#define SUBPROCESS_HPP

#include "subprocess.h"
#include <streambuf>
#include <iostream>

class FileOstreamBuf: public std::streambuf
{
public:
    FILE* fp;
    FileOstreamBuf(FILE* tfp):fp(tfp){}
    virtual int overflow(int c=traits_type::eof())
    {
        if(c == std::streambuf::traits_type::eof() || ferror (fp) ) return std::streambuf::traits_type::eof();
        return fputc(c,fp);
    }
    virtual std::streamsize xsputn(const char* s,std::streamsize n)
    {
        return fwrite(s,n,1,fp);
    }
};

template<
subprocess_weak unsigned
    (*SPREAD)(struct subprocess_s *const process, 
              char *const buffer,unsigned size)
>
class SubprocessIstreamBuf: public std::streambuf
{
public:
    struct subprocess_s* process_ptr;
    SubprocessIstreamBuf(struct subprocess_s* tpp):process_ptr(tpp)
    {}
    virtual int underflow()
    {
        char ch;
        return SPREAD(process_ptr,&ch,1) ? ch : std::streambuf::traits_type::eof();
    }
    virtual std::streamsize xsgetn(char* s,std::streamsize n)
    {
        return SPREAD(process_ptr,s,n);
    }
};

struct Subprocess
{
private:
    FILE* init(const char *const commandLine[])
    {
        if(subprocess_create(commandLine, subprocess_option_enable_async | subprocess_option_inherit_environment | subprocess_option_no_window, &process) != 0)
        {
            m_good=false;
            return NULL;
        }
        return subprocess_stdin(&process);
    }

	struct subprocess_s process;
public:
    bool m_good;
        
    std::ostream instream;
    std::istream outstream;
    std::istream errstream;
    
    
    operator bool() const{ return m_good; }
    
    
    Subprocess(const char *const commandLine[]):
        m_good(true),
        instream(new FileOstreamBuf(init(commandLine))),
        outstream(new SubprocessIstreamBuf<subprocess_read_stdout>(&process)),
        errstream(new SubprocessIstreamBuf<subprocess_read_stderr>(&process))
    {
        
    }
    void terminate();
    int join()
    {
        int ret=-1;
        if(m_good && subprocess_alive(&process)) subprocess_join(&process, &ret);
        return ret;
    }
    ~Subprocess()
    {
        terminate();
        join();
        if(m_good)
        {
            subprocess_destroy(&process);
        }
    }
};



#endif
