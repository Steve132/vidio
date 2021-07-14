#include <iostream>
#include "subprocess.hpp"

int main(int argc,char** argv)
{
    std::cout << "Hello, world" << std::endl;
    const char* cmd[]={"/usr/bin/ffmpeg",0}; // works if don't use errstream
	//const char *const cmd[] = {"/usr/bin/ffmpeg", "-v","24","-pix_fmts", 0};
    //const char* cmd[] = {"/usr/bin/ffmpeg", "-i", "test.mp4", "-pix_fmt", "rgba", "-vcodec", "rawvideo", "-f", "image2pipe", "pipe:1", 0};
    Subprocess sp{cmd};
	/*if(sp.errstream.good())
		std::cout << "errorstream good!" << std::endl;
    std::cout << sp.errstream.rdbuf();
	*/
	if(sp.outstream.good())
		std::cout << "outstream good!" << std::endl;
    std::cout << sp.outstream.rdbuf();
    return 0;
}
