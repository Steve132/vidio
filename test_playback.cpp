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
		vidio::Size sz;
		sz.width = 1280;
		sz.height = 720;
		vidio::Reader reader(std::string(argv[1]),"");
		CImg<unsigned char> visu(sz.width,sz.height,1,3,0);
		CImgDisplay main_disp(visu,"preview");
//		cerr << "frame size: " << reader.video_frame_dimensions().width << "x" << reader.video_frame_dimensions().height << endl;
		double fps = reader.framerate();
		cerr << "fps: " << fps << endl;
		cerr << "video frame bufsize: " << reader.video_frame_bufsize() << endl;
		if(reader.video_frame_bufsize() > sz.width*sz.height*(24/8))
		{
			throw std::runtime_error("frame bufsize not initialized properly");
		}
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
			
		/*for(size_t i=0;reader.read(framebuf.get());i++)
		{
			cout << "Num Frames: " << i << "\n";
			//for(int c=0;c<reader.frame_buffer_size;c++)
			//{
			//	cout << (int)framebuf[c] << " ";
			//}
		}*/
	} 
	catch(const std::exception& e)
	{
		std::cerr << "There was an exception! " << e.what() << endl;
	}
	return 0;
}
