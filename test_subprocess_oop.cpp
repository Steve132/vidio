#include <iostream>
#include "subprocess.hpp"

int main(int argc,char** argv)
{
    std::cout << "Hello, world" << std::endl;
    const char* cmd[]={"/usr/bin/ffmpeg",0};
    Subprocess sp{cmd};
    std::cout << sp.errstream.rdbuf();
    return 0;
}
