#include<iostream>
#include<string>
#include"vidio.hpp"
#include<exception>
#include <CImg.h>
using namespace cimg_library;
using namespace std;


int main(int argc,char** argv)
{
	try
	{
		vidio::Reader reader(argv[1],vidio::Size(720,480),3,1);
		CImg<unsigned char> visu(720,480,1,3,0);
		CImgDisplay main_disp(visu,"preview");
		std::unique_ptr<uint8_t[]> framebuf(new uint8_t[reader.frame_buffer_size]);

		cerr << reader.frame_buffer_size << "EEE\n";
		main_disp.show();
		while (!main_disp.is_closed() && reader.read(framebuf.get()))
		{
			main_disp.wait(33);
			visu.assign(framebuf.get(),3,720,480,1);
			visu.permute_axes("yzcx");
			main_disp.display(visu);
		}
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
