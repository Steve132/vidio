#include "named_pipe.hpp"
#include <iostream>
#include <string>
#include <exception>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <unistd.h> // sleep()
using namespace std;

void test_writer(const std::string& filename, std::size_t bufsize = 8)
{
	named_pipe::Writer writer(filename);
	std::unique_ptr<uint8_t[]> framebuf(new uint8_t[bufsize]);

	// Initialize test buffer
	uint8_t ch = 's';
	for(size_t i = 0; i < bufsize; i++)
	{
		framebuf.get()[i] = ch;
	}

	uint8_t depth = 8;
	while (1)
	{
		framebuf.get()[0] = depth;
		writer.write_buf(framebuf.get(), bufsize);
		sleep(1);

		if(depth == 0)
		{
			break;
		}
		depth--;
	}
}

void test_reader(const std::string& filename, std::size_t bufsize = 8)
{
	named_pipe::Reader reader(filename);
	std::unique_ptr<uint8_t[]> framebuf(new uint8_t[bufsize]);

	while (1)
	{
		try
		{
			reader.read_buf(framebuf.get(), bufsize);
		}
		catch(const std::exception& e)
		{
			std::cerr << "There was an exception! " << e.what() << endl;
		}
		//printf("User1: %d\n", framebuf.get()[0]);
		cout << static_cast<int>(framebuf.get()[0]) << endl;
		if(framebuf.get()[0] == 0)
		{
			break;
		}
	}
}

int main(int argc, char** argv)
{
	if(argc < 3)
	{
		cout << "Usage:" << endl;
		cout << "Open 2 prompts." << endl;
		cout << "In one prompt, open the writer pipe: " << endl << "./test_named_pipe <filename> 0" << endl;
		cout << "In other prompt, open the reader pipe: " << endl << "./test_named_pipe <filename> 1" << endl;
		return -1;
	}
	std::string filename = std::string(argv[1]);
	int is_reader = atoi(argv[2]);
	try
	{
		if(is_reader)
		{
			test_reader(filename);
		}
		else
		{
			test_writer(filename);
		}
	} 
	catch(const std::exception& e)
	{
		std::cerr << "There was an exception! " << e.what() << endl;
	}
	return 0;
}
