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

	cout << "buf being written:" << endl;
	uint8_t ch = 's';
	for(size_t i = 0; i < bufsize; i++)
	{
		framebuf.get()[i] = ch;
		cout << framebuf.get()[i] << endl;
	}
	cout.flush();

	int iter = 10;
	while (1)
	{
		cout << "writing iter: " << iter << endl;
		writer.write_buf(framebuf.get(), bufsize);
		sleep(1); // let the reader get the data.  TODO: O_NONBLOCK in writer/reader?  async?

		if(iter < 0)
		{
			break;
		}
		iter --;
	}
}

void test_reader(const std::string& filename, std::size_t bufsize = 8)
{
	named_pipe::Reader reader(filename);
	std::unique_ptr<uint8_t[]> framebuf(new uint8_t[bufsize]);

	std::size_t niters = 3;
	for(size_t i = 0; reader.read_buf(framebuf.get(), bufsize) && i < niters;i++)
	{
		cout << "reading iter: " << i << endl;
		for(int c = 0; c < bufsize; c++)
		{
			cout << (int)framebuf[c] << " ";
		}
		cout << endl;
	}
}

void test_writer_no_named_pipe_class(const std::string& filename, std::size_t bufsize = 8)
{
	int fd;

	// FIFO file path
	const char* myfifo = filename.c_str();

	// Creating the named file(FIFO)
	// mkfifo(<pathname>, <permission>)
	mkfifo(myfifo, 0666);

	// Initialize test buffer
	std::unique_ptr<uint8_t[]> framebuf(new uint8_t[bufsize]);
	uint8_t ch = 's';
	for(size_t i = 0; i < bufsize; i++)
	{
		framebuf.get()[i] = ch;
		cout << framebuf.get()[i] << endl;
	}

	uint8_t depth = 8;
	while (1)
	{
		uint8_t* tmpbuf = static_cast<uint8_t*>(framebuf.get());
		fd = open(myfifo, O_WRONLY);
		tmpbuf[0] = depth;
		write(fd, tmpbuf, bufsize);
		close(fd);
		sleep(1);

		if(depth == 0)
		{
			break;
		}
		depth--;
	}
}

void test_reader_no_named_pipe_class(const std::string& filename, std::size_t bufsize = 8)
{
	int fd1;

	// FIFO file path
	const char* myfifo = filename.c_str();

	// Creating the named file(FIFO)
	// mkfifo(<pathname>,<permission>)
	mkfifo(myfifo, 0666);

	std::unique_ptr<uint8_t[]> framebuf(new uint8_t[bufsize]);

	while (1)
	{
		// First open in read only and read
		fd1 = open(myfifo,O_RDONLY);
		uint8_t* tmpbuf = static_cast<uint8_t*>(framebuf.get());
		read(fd1, tmpbuf, bufsize);

		// Print the read string and close
		printf("User1: %d\n", tmpbuf[0]);
		close(fd1);

		if(tmpbuf[0] == 0)
		{
			break;
		}
	}
	unlink(myfifo);
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
			//test_reader(filename);
			test_reader_no_named_pipe_class(filename);
		}
		else
		{
			//test_writer(filename);
			test_writer_no_named_pipe_class(filename);
		}
	} 
	catch(const std::exception& e)
	{
		std::cerr << "There was an exception! " << e.what() << endl;
	}
	return 0;
}
