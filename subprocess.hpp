#ifndef SUBPROCESS_HPP
#define SUBPROCESS_HPP

#include "subprocess.h"
#include <streambuf>
#include <iostream>


#ifdef STREAM_INTERFACE
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

#endif

struct Subprocess
{
private:
    FILE* init(const char *const commandLine[])
    {
        m_good=true;
        if(subprocess_create(commandLine, subprocess_option_enable_async | subprocess_option_inherit_environment | subprocess_option_no_window, &process) != 0)
        {
            m_good=false;
            return NULL;
        }
        return subprocess_stdin(&process);
    }

    bool m_good;
	struct subprocess_s process;
    FILE* input_fp;
public:
    operator bool() const{ return m_good; }
    
    unsigned int write_to_stdin(const char* buf,unsigned int n)
    {
        return fwrite(buf,n,1,input_fp);
    }
    unsigned int read_from_stdout(char* buf,unsigned int n)
    {
        return subprocess_read_stdout(&process,buf,n);
    }
    unsigned int read_from_stderr(char* buf,unsigned int n)
    {
        return subprocess_read_stderr(&process,buf,n);
    }
    
    Subprocess(const char *const commandLine[]):
        m_good(true),input_fp(init(commandLine))
    {}
    void terminate()
    {
       // if(m_good &&subprocess_alive(&process)) subprocess_terminate(&process,&ret);
    }
    int join()
    {
        int ret=-1;
        if(m_good && subprocess_alive(&process)) subprocess_join(&process, &ret);
        return ret;
    }
    ~Subprocess()
    {
        terminate();
        //join();
        if(m_good)
        {
            subprocess_destroy(&process);
        }
    }
};



#endif

