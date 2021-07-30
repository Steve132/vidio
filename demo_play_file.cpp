
#include<iostream>
#include "CImg.h"
#include <vidio.hpp>
#include <vector>
#include <cstdint>
using namespace cimg_library;



int main(int argc,char** argv) {
    if(argc < 2)
    {
        std:: cerr << "Error, no file specified." << std::endl;
        return -1;
    }
    vidio::Reader vidfile(argv[1],"rgb24");
    auto dims=vidfile.video_frame_dimensions();
    std::vector<uint8_t> framebuf(vidfile.video_frame_bufsize());
    std::cout << "sz:" << dims.width << "x" << dims.height << ":" << vidfile.pixelformat().bits_per_pixel << std::endl;
    
    CImg<unsigned char> image(dims.width,dims.height,1,3);
    
    CImgDisplay main_disp(image);
    
    while (!main_disp.is_closed() && vidfile.read_video_frame(framebuf.data())) {
        image.assign(framebuf.data(),3,dims.width,dims.height,1);
        
        image.permute_axes("yzcx"); //CImg requires images in a different layout.
        main_disp=image;
        main_disp.wait(1000.0/vidfile.framerate());
    }
    return 0;
}
