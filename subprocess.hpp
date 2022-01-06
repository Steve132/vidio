#ifndef SUBPROCESS_HPP
#define SUBPROCESS_HPP

#include "subprocess.h"
#include <stdexcept>
#include <cstdio>

struct Subprocess
{
public:
    enum BehaviorOnDestruct
    {
        JOIN,
        TERMINATE,
        NONE
    };
private:
    struct subprocess_s process;
    BehaviorOnDestruct destruct_behavior;
public:
    FILE *input;
    Subprocess(const char *const commandLine[],BehaviorOnDestruct tdb=TERMINATE):input(nullptr),destruct_behavior(tdb)
    {
        if(subprocess_create(commandLine, subprocess_option_enable_async | subprocess_option_inherit_environment | subprocess_option_no_window, &process) != 0)
        {
            throw std::runtime_error("Could not run command.");
        }
        this->input=subprocess_stdin(&process);
    }
	unsigned int write_to_stdin(const char* buf,unsigned int n)
	{
<<<<<<< HEAD
		FILE* inp=subprocess_stdin(&process);
		return (unsigned int)fwrite(buf, n,1,inp);
=======
		return (unsigned int)fwrite(buf, n,1,input);
>>>>>>> master
	}
    
    unsigned int read_from_stdout(char* buf,unsigned int n)
    {
        return subprocess_read_stdout(&process,buf,n);
    }
    unsigned int read_from_stderr(char* buf,unsigned int n)
    {
        return subprocess_read_stderr(&process,buf,n);
    }
    void terminate()
    {
       if(subprocess_alive(&process)) 
           subprocess_terminate(&process);
    }
    int join()
    {
        int ret=-1;
        subprocess_join(&process, &ret);
        return ret;
    }
    ~Subprocess()
    {
        switch(destruct_behavior)
        {
            case JOIN:
            {
                join(); break;
            }
            case TERMINATE:
            {
                terminate(); break;
            }
            default:
            {
                break;
            }
        };
        subprocess_destroy(&process);
    }
    
    Subprocess& operator=(const Subprocess&)=delete;
    Subprocess& operator=(Subprocess&&)=delete;
    Subprocess(const Subprocess&)=delete;
    Subprocess(Subprocess&&)=delete;
};



#endif

