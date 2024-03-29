#include <iostream>
#include "subprocess.hpp"
#include <cstring>

// Note: this works even for ffmpeg with image2pipe to get err and out stream!
int manual_subprocess(const char* const* cmd)
{
	struct subprocess_s process;
	if(subprocess_create(cmd, subprocess_option_enable_async | subprocess_option_inherit_environment | subprocess_option_no_window, &process) != 0)
		return -1;

	const int nchars = 128, nerrchars = nchars * 2;
	char buf[nchars], errbuf[nerrchars];
	memset(buf, '\0', nchars);
	memset(errbuf, '\0', nerrchars);
	int ret2 = subprocess_read_stderr(&process, errbuf, nerrchars);
	if(ret2 == 0)
	{
		std::cout << "subprocess_read_stderr failure." << std::endl;
		return -1;
	}
	std::cout << "err ret:" << ret2 << std::endl;
	std::cout << "errbuf:" << errbuf << std::endl;
	int ret = subprocess_read_stdout(&process, buf, nchars); // struct subprocess_s *const process, char *const buffer,unsigned size);
	if(ret == 0) // TODO: this handling is missing in struct initialization for SPREAD
	{
		std::cout << "subprocess_read_stdout failure." << std::endl;
		return -1;
	}
	std::cout << "out ret:" << ret << std::endl;
	std::cout << "out buf:" << buf << std::endl;
	subprocess_destroy(&process);
	return 0;
}

int manual_subprocess_class(const char* const* cmd)
{
    Subprocess sp{cmd};

	const int nchars = 128, nerrchars = nchars * 2;
	char buf[nchars], errbuf[nerrchars];
	memset(buf, '\0', nchars);
	memset(errbuf, '\0', nerrchars);
	int ret2 = sp.read_from_stderr(errbuf, nerrchars);
	if(ret2 == 0)
	{
		std::cout << "subprocess_read_stderr completion of process, no data read." << std::endl;
		return -1;
	}
	std::cout << "err ret:" << ret2 << std::endl;
	std::cout << "errbuf:" << errbuf << std::endl;
	int ret = sp.read_from_stdout(buf, nchars); // struct subprocess_s *const process, char *const buffer,unsigned size);
	if(ret == 0) // TODO: this handling is missing in struct initialization for SPREAD
	{
		std::cout << "subprocess_read_stdout completion of process, no data read." << std::endl;
		return -1;
	}
	std::cout << "out ret:" << ret << std::endl;
	std::cout << "out buf:" << buf << std::endl;
	return 0;
}

int main(int argc,char** argv)
{
    std::cout << "Hello, world" << std::endl;
    //const char* cmd[]={"/usr/bin/ffmpeg",0}; // works if don't use errstream
	//const char *const cmd[] = {"/usr/bin/ffmpeg", "-v","24","-pix_fmts", 0};
    const char* cmd[] = {"/usr/bin/ffmpeg", "-i", "test.mp4", "-pix_fmt", "rgba", "-vcodec", "rawvideo", "-f", "rawvideo", "-", 0};


	manual_subprocess_class(cmd);

    return 0;
}
