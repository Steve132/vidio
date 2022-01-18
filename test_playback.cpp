#include<iostream>
#include<string>
#include"vidio.hpp"
#include<exception>
#include<sstream>
#include "CImg.h"
using namespace cimg_library;
using namespace std;

// Standalone write example works:
void manual_write()
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


int main(int argc,char** argv)
{
	try
	{
		//manual_write();
		//return 0;

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
