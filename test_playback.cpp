#include<iostream>
#include<string>
#include"vidio.hpp"
#include"subprocess.hpp"
#include<exception>
#include<sstream>
#include "CImg.h"
using namespace cimg_library;
using namespace std;

// Standalone write example works:
void manual_write_pipe()
{
	std::stringstream ss;
	ss << "ffmpeg -hide_banner -y -f rawvideo -vcodec rawvideo -pix_fmt rgba -s 1280x720 -r 25 -i - out.mp4";
	FILE* pipeout = popen(ss.str().c_str(), "w");
	size_t correctSize = 1280*720*4; //width * height * nchannels;
	std::unique_ptr<uint8_t[]> framebuf(new uint8_t[correctSize]);
	for(size_t i = 0; i < correctSize; i++)
	{
		framebuf.get()[i] = i % 256; // set to a value.
	}
	
	if( pipeout )
	{
		size_t writtenCount=fwrite(framebuf.get(), 1, correctSize, pipeout);
		fflush( pipeout );
		if(writtenCount==correctSize)
			std::cout << "wrote bytes successfully." << std::endl;
	}

	if(pipeout != NULL)
	{
		fflush(pipeout);
		pclose(pipeout);
	}
}

void write_video_frame(std::unique_ptr<Subprocess>& ffmpeg_proc, const void* buf, const size_t correctSize)
{
    if( ffmpeg_proc->input )
    {
        const char ch = 's';
        //const uint8_t* tmpbuf1 = static_cast<const uint8_t*>(buf);
        //size_t writtenCount=fwrite(reinterpret_cast<const char*>(tmpbuf1), 1, correctSize, ffmpeg_proc->input); // Can't write whole frame buffer at one time (due to async Subprocess?)
        size_t writtenCount=fwrite(&ch, 1, 1, ffmpeg_proc->input);
        if(writtenCount==1)//correctSize)
        {
            std::cout << "wrote bytes successfully." << std::endl;
            if(ferror(ffmpeg_proc->input))
            {
                perror("Error:");
                std::cout << "Error with write pipe!" << std::endl;
            }
            fflush(ffmpeg_proc->input); // // SIGPIPE Broken!  Is the command not initializing properly?  ONLY FLUSH once per frame?
        }
    }
}

// Standalone write example w/Subprocess: TODO: doesn't work!
void manual_write_subprocess()
{
	//std::stringstream ss;
	//ss << "ffmpeg -hide_banner -y -f rawvideo -vcodec rawvideo -pix_fmt rgba -s 1280x720 -r 25 -i - out.mp4";
    const char *const commandLine[] = {"ffmpeg", "-hide_banner", "-y", "-f", "rawvideo", "-vcodec", "rawvideo", "-pix_fmt", "rgba", "-s", "1280x720", "-r", "25", "-i", "-", "out.mp4", 0};
    //const char *const commandLine[] = {"ffmpeg -hide_banner -y -f rawvideo -vcodec rawvideo -pix_fmt rgba -s 1280x720 -r 25 -i - out.mp4", 0};
    std::unique_ptr<Subprocess> ffmpeg_proc=std::make_unique<Subprocess>(commandLine, Subprocess::JOIN);
	FILE* pipeout = ffmpeg_proc->input;//popen(ss.str().c_str(), "w");
	size_t correctSize = 1280*720*4; //width * height * nchannels;
	std::unique_ptr<uint8_t[]> framebuf(new uint8_t[correctSize]);
	for(size_t i = 0; i < correctSize; i++)
	{
        framebuf.get()[i] = 1; // set to a value.
	}

    //write_video_frame(ffmpeg_proc, framebuf.get(), correctSize);
    const char ch = 's';
    size_t writtenCount=fwrite(&ch, 1, 1, ffmpeg_proc->input);

    // From subprocess.h read method:
    //const int fd = fileno(ffmpeg_proc->input); // broken pipe!
    //size_t writtenCount = write(fd, &ch, 1); // broken pipe!

    fflush(ffmpeg_proc->input); // broken pipe!

    if(pipeout != NULL)
    {
        fflush(pipeout);
        //pclose(pipeout); // handled by Subprocess
    }
	
}

int main(int argc,char** argv)
{
	try
	{
        //manual_write_pipe();
        manual_write_subprocess();
		return 0;

		vidio::Reader reader(std::string(argv[1]),"rgba");
		double fps = reader.framerate();
		vidio::Size sz = reader.video_frame_dimensions();
		CImg<unsigned char> visu(sz.width,sz.height,1,4,0);
		CImgDisplay main_disp(visu,"preview");
		cerr << "frame size: " << reader.video_frame_dimensions().width << "x" << reader.video_frame_dimensions().height << endl;
		std::unique_ptr<uint8_t[]> framebuf(new uint8_t[reader.video_frame_bufsize()]);

		main_disp.show();
		/*while (!main_disp.is_closed() && reader.read(framebuf.get()))
		{
			main_disp.wait(33);
			visu.assign(framebuf.get(),3,sz.width,sz.height,1);
			visu.permute_axes("yzcx");
			main_disp.display(visu);
		}
		*/
		main_disp.close();

		size_t nframes = 2*fps;
		for(size_t i=0;reader.read_video_frame(framebuf.get()) && i < nframes;i++)
		{
			//for(int c=0;c<reader.frame_buffer_size;c++)
			//{
			//	cout << (int)framebuf[c] << " ";
			//}
		}

		vidio::Writer writer("out.mp4",sz,fps,"rgba");
		// Just write the last frame read in.  Later, look at weaving write into read loop above.
		std::unique_ptr<uint8_t[]> write_framebuf(new uint8_t[reader.video_frame_bufsize()]);
		writer.write_video_frame(write_framebuf.get());	
	} 
	catch(const std::exception& e)
	{
		std::cerr << "There was an exception! " << e.what() << endl;
	}
	return 0;
}
