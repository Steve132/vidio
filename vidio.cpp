#include "vidio.hpp"
#include "subprocess.h"
#include<unordered_map>
#include<vector>
#include<cstdio>
#include<iostream>


class FileOstreamBuf: public std::streambuf
{
public:
    FILE* fp;
    FileOstreamBuf(FILE* tfp):fp(tfp){}
    virtual int overflow(int c=traits_type::eof())
    {
        if(c == std::streambuf::traits_type::eof() || ferror (fp) ) return std::streambuf::traits_type::eof();
        return fputc(c,fp);
    }
    virtual std::streamsize xsputn(const char* s,std::streamsize n)
    {
        return fwrite(s,n,1,fp);
    }
};

template<
subprocess_weak unsigned
    (*SPREAD)(struct subprocess_s *const process, 
              char *const buffer,unsigned size)
>
class SubprocessIstreamBuf: public std::streambuf
{
public:
    struct subprocess_s* process_ptr;
    SubprocessIstreamBuf(struct subprocess_s* tpp):process_ptr(tpp)
    {}
    virtual int underflow()
    {
        char ch;
        return SPREAD(process_ptr,&ch,1) ? ch : std::streambuf::traits_type::eof();
    }
    virtual std::streamsize xsgetn(char* s,std::streamsize n)
    {
        return SPREAD(process_ptr,s,n);
    }
};

struct Subprocess
{
private:
    FILE* init(const char *const commandLine[])
    {
        if(subprocess_create(commandLine, subprocess_option_enable_async | subprocess_option_inherit_environment | subprocess_option_no_window, &process) != 0)
        {
            m_good=false;
            return NULL;
        }
        return subprocess_stdin(&process);
    }

	struct subprocess_s process;
public:
    bool m_good;
        
    std::ostream instream;
    std::istream outstream;
    std::istream errstream;
    
    
    operator bool() const{ return m_good; }
    
    
    Subprocess(const char *const commandLine[]):
        m_good(true),
        instream(new FileOstreamBuf(init(commandLine))),
        outstream(new SubprocessIstreamBuf<subprocess_read_stdout>(&process)),
        errstream(new SubprocessIstreamBuf<subprocess_read_stderr>(&process))
    {
        
    }
    
    int join()
    {
        int ret=-1;
        if(m_good && subprocess_alive(&process)) subprocess_join(&process, &ret);
        return ret;
    }
    ~Subprocess()
    {
        join();
        if(m_good)
        {
            subprocess_destroy(&process);
        }
    }
};



//the default loglevel should 

bool parse_ffmpeg_pixfmts(
    std::unordered_map<std::string,vidio::PixelFormat>& input_pixel_formats, 
    std::unordered_map<std::string,vidio::PixelFormat>& output_pixel_formats, 
    const std::string& default_ffmpeg_path = "/usr/bin/ffmpeg")
{
	const char *const commandLine[] = {default_ffmpeg_path.c_str(), "-v","24","-pix_fmts", 0};


    Subprocess ffproc(commandLine);
    if(!ffproc)
    {
        return false;
    }

	input_pixel_formats.clear();
	output_pixel_formats.clear();
	bool line_is_a_format = false;
	while(ffproc.outstream)
	{
		std::string temps;
        std::getline(ffproc.outstream,temps);
		if(line_is_a_format) // IO... yuv420p                3            12
		{
			vidio::PixelFormat pixfmt;
			size_t pixfmt_ind = temps.find(" ", 6); // skip "IO... "
			if(pixfmt_ind != std::string::npos)
			{
				pixfmt.name = temps.substr(6,pixfmt_ind-6);
			}
			size_t bpp_ind = temps.rfind(" ");
			if(bpp_ind != std::string::npos)
			{
				pixfmt.bits_per_pixel = stoi(temps.substr(bpp_ind));
			}
			if((pixfmt_ind != std::string::npos) && (bpp_ind != std::string::npos))
			{
				pixfmt.num_components = stoi(temps.substr(pixfmt_ind + 1, bpp_ind - (pixfmt_ind + 1)));

				if(temps[0] == 'I')
				{
					input_pixel_formats[pixfmt.name]=pixfmt;
				}
				if(temps[1] == 'O')
				{
					output_pixel_formats[pixfmt.name]=pixfmt;
				}
			}
		}
		if(temps.find("FLAGS") != std::string::npos)
		{
			std::getline(ffproc.outstream,temps); // ignore next line.
			line_is_a_format = true;
		}
	}
	
	return ffproc.join()==0;
}

namespace vidio
{

class FFMPEG_Install::Impl
{
public:
	
	bool good() const {return m_good;}

	bool m_good;
	std::string ffmpeg_path;
	std::unordered_map<std::string,vidio::PixelFormat> readable_formats; //"O"
	std::unordered_map<std::string,vidio::PixelFormat> writeable_formats; //"I"
    Impl(const std::vector<std::string>& additional_search_locations={})
    {
        // call pix_fmts with default install location and if it doesn't work, test additional_search_locations
        std::vector<std::string> platform_defaults = {"/usr/bin/ffmpeg"};
        std::string ffmpeg_path = "";
        platform_defaults.insert(platform_defaults.end(),additional_search_locations.begin(),additional_search_locations.end());
        m_good = false;
        
        std::unordered_map<std::string,PixelFormat> input_pixel_formats,output_pixel_formats;
        for(
            std::vector<std::string>::const_iterator search_location = additional_search_locations.begin(); 
        !m_good && search_location != additional_search_locations.end(); search_location++)
        {
            m_good = parse_ffmpeg_pixfmts(input_pixel_formats, output_pixel_formats, *search_location);
            if(m_good)
            {
                ffmpeg_path = *search_location;
            }
        }
        if(m_good)
        {
            writeable_formats=input_pixel_formats;
            readable_formats=output_pixel_formats;
        }
        else
        {
            throw std::runtime_error("Unable to find ffmpeg binary");
        }
}

    
};


const std::unordered_map<std::string,PixelFormat>&  FFMPEG_Install::valid_read_pixelformats() const
{
    return impl->readable_formats;
}
const std::unordered_map<std::string,PixelFormat>&  FFMPEG_Install::valid_write_pixelformats() const
{
    return impl->writeable_formats;
}




//eventually use HW formats. https://www.ffmpeg.org/doxygen/2.5/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5

class Reader::Impl
{
public:
    Impl(const std::string& filename,const std::string& pixelformat,const FFMPEG_Install& install)
    {
        filepointer=NULL;
        //TODO open filepointer here.  parse fps and pixelformat string. 
        if(pixelformat == "") { 
            //pixelformat=parsed_pixelformat;
        }
        //fmt=
    }
    FILE* filepointer;
    PixelFormat fmt;
    const PixelFormat& pixelformat() const
    {
        return fmt;
    }
    double fps;
	double framerate() const
	{
        return fps;
    }
	bool good() const
	{
        return filepointer && ferror(filepointer);
    }
    Size frame_dims;
	Size video_frame_dimensions() const
	{
        return frame_dims;
    }

	bool read_video_frame(void *buf) const
	{
        return false;
    }
	bool read_audio_frame(void* buf) const
	{
        return false;
    }
};




Reader::Reader(const std::string& filename,const std::string& pixelformat,const FFMPEG_Install& install):
    impl(std::make_shared<Reader::Impl>(filename,pixelformat,install))
{}

const PixelFormat& Reader::pixelformat() const
{
    return impl->pixelformat();
}
double Reader::framerate() const
{
    return impl->framerate();
}
bool Reader::good() const
{
    return impl && impl->good();
}
Size Reader::video_frame_dimensions() const
{
    return impl->video_frame_dimensions();
}

bool Reader::read_video_frame(void *buf) const
{
    return impl->read_video_frame(buf);
}
bool Reader::read_audio_frame(void* buf) const
{
    return impl->read_audio_frame(buf);
}
















class Writer::Impl
{
public:

};



}
