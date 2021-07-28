#include<iostream>
#include<string>
#include"vidio.hpp"
#include<exception>
#include "CImg.h"
using namespace cimg_library;
using namespace std;


int main(int argc,char** argv)
{
	try
	{
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
	} 
	catch(const std::exception& e)
	{
		std::cerr << "There was an exception! " << e.what() << endl;
	}
	return 0;
}
