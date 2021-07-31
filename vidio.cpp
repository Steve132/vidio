#include "vidio.hpp"
#include "subprocess.h"
#include<unordered_map>
#include<vector>
#include<cstdio>
#include<iostream>
#include<sstream>
#include<cstring>
#include "subprocess.hpp"
using namespace std;


static bool getline_subprocess_stderr(Subprocess& proc,std::string& result,int endch='\n')
{
    result.clear();
    result.reserve(384);
    char ch;
    unsigned int ret;
    for(ret=proc.read_from_stderr(&ch,1);ret > 0 && ch!=endch;ret=proc.read_from_stderr(&ch,1))
    {
        result+=ch;
    }
    return ret > 0;
}
static bool getline_subprocess_stdout(Subprocess& proc,std::string& result,int endch='\n')
{
    result.clear();
    result.reserve(384);
    char ch;
    unsigned int ret;
    for(ret=proc.read_from_stdout(&ch,1);ret > 0 && ch!=endch;ret=proc.read_from_stdout(&ch,1))
    {
        result+=ch;
    }
    return ret > 0;
}

//the default loglevel should 

bool parse_ffmpeg_pixfmts(
    std::unordered_map<std::string,vidio::PixelFormat>& input_pixel_formats, 
    std::unordered_map<std::string,vidio::PixelFormat>& output_pixel_formats, 
    const std::string& default_ffmpeg_path = "/usr/bin/ffmpeg")
{
	const char *const commandLine[] = {default_ffmpeg_path.c_str(), "-hide_banner","-v","24","-pix_fmts", 0};

    Subprocess ffproc(commandLine); // TODO: Fix Subprocess initialization!
	
	input_pixel_formats.clear();
	output_pixel_formats.clear();
	bool line_is_a_format = false;
    std::string temps;
	while(getline_subprocess_stdout(ffproc,temps))
	{
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
			getline_subprocess_stdout(ffproc,temps); // ignore next line.
			line_is_a_format = true;
		}
	}

	return ffproc.join()==0;
}

    std::unique_ptr<Subprocess> parse_input_pixel_fmt_and_open(const std::vector<std::string>& ffmpeg_input_args,const std::string& pixelformat,const vidio::FFMPEG_Install& install, std::string& parsed_pixelformat, double& parsed_fps, vidio::Size& parsed_frame_dimensions)
{
	if(!install.good())
	{
		throw std::runtime_error("FFMPEG not found.");
	}
	
	std::vector<const char*> cmdLine;
    cmdLine.emplace_back(install.ffmpeg_path().c_str());
    cmdLine.emplace_back("-hide_banner");
    
	for(const std::string& ia : ffmpeg_input_args)
    {
        cmdLine.push_back(ia.c_str());
    }
    const char *const commandLinePostfix[] = {"-vcodec", "rawvideo", "-f", "rawvideo", "-pix_fmt", (pixelformat == "" ? "rgb24" : pixelformat.c_str()),  "-", 0};
	
    for(const char* const* clP=commandLinePostfix;*clP!=0;clP++)
    {
        cmdLine.push_back(*clP);
    }
	cmdLine.push_back(0);
    std::unique_ptr<Subprocess> ffmpeg_proc=std::make_unique<Subprocess>(cmdLine.data());
	
	std::string temps;

	int width = -1, height = -1;
	double fps = -1;
	string vid_fmt = "";
	while(fps == -1)
	{
		if(!getline_subprocess_stderr(*ffmpeg_proc,temps))
		{
			break; // end of err stream.
		}
		// look for format ex: Stream #0:0(und): Video: h264 (Main) (avc1 / 0x31637661), yuv420p, 1280x720 [SAR 1:1 DAR 16:9], 862 kb/s, 25 fps, 25 tbr, 12800 tbn, 50 tbc (default)
		size_t vind = temps.find("Video:");
		if((temps.find("Stream #0") != string::npos &&
			vind != string::npos))
		{
			size_t start_fmt_ind = temps.find(",", vind)+1;
			size_t end_fmt_ind = temps.find(",", start_fmt_ind);
			if((start_fmt_ind != string::npos &&
				end_fmt_ind != string::npos))
			{
				vid_fmt = temps.substr(start_fmt_ind+1, end_fmt_ind-(start_fmt_ind+1));
				size_t end_aspect_ratio_ind = temps.find(" ", end_fmt_ind+2);
				if(end_aspect_ratio_ind != string::npos)
				{
					string aspect_ratio = temps.substr(end_fmt_ind+2, end_aspect_ratio_ind - (end_fmt_ind+1));
					size_t xind = aspect_ratio.find("x");
					if(xind != string::npos)
					{
						width = stoi(aspect_ratio.substr(0,xind));
						height = stoi(aspect_ratio.substr(xind+1));
					}
				}
			}
			size_t fps_ind = temps.find("fps");
			if(fps_ind != string::npos)
			{
				size_t start_fps_ind = temps.rfind(",", fps_ind) + 2;
				if(start_fps_ind != string::npos)
				{
					fps = stod(temps.substr(start_fps_ind, fps_ind-start_fps_ind));
				}
			}
			break;
		}
	}
	//cerr << "Detected video format for: " << filename << ": " << vid_fmt << " " << fps << "fps " << width << "x" << height << endl;
	parsed_pixelformat = vid_fmt;
	parsed_fps = fps;
	parsed_frame_dimensions.width = width;
	parsed_frame_dimensions.height = height;
	return ffmpeg_proc;
}



namespace vidio
{

class FFMPEG_Install::Impl
{
public:
	
	bool good() const {return m_good;}
	const std::string& ffmpeg_path() const {return str_ffmpeg_path;}

	bool m_good;
	std::string str_ffmpeg_path;
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
            std::vector<std::string>::const_iterator search_location = platform_defaults.begin(); 
        !m_good && search_location != platform_defaults.end(); search_location++)
        {
            m_good = parse_ffmpeg_pixfmts(input_pixel_formats, output_pixel_formats, *search_location);
            if(m_good)
            {
                str_ffmpeg_path = *search_location;
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

const std::string& FFMPEG_Install::ffmpeg_path() const
{
	return impl->ffmpeg_path();
}

bool FFMPEG_Install::good() const
{
	return impl->m_good;
}

FFMPEG_Install::FFMPEG_Install(const std::vector<std::string>& additional_search_locations):
	impl(std::make_shared<FFMPEG_Install::Impl>(additional_search_locations))
{}

FFMPEG_Install::~FFMPEG_Install()
{}


//eventually use HW formats. https://www.ffmpeg.org/doxygen/2.5/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5

class Reader::Impl
{
public:
    std::unique_ptr<Subprocess> ffmpeg_process;
    Impl(const std::vector<std::string>& ffmpeg_input_args,const std::string& pixelformat,const FFMPEG_Install& install)
    {
		if(!install.good())
		{
			throw std::runtime_error("FFMPEG Installation not found.");
		}
		std::string parsed_pixelformat = "";
		double parsed_fps = -1;
		vidio::Size parsed_frame_dimensions;
        try
        {
            ffmpeg_process=parse_input_pixel_fmt_and_open(ffmpeg_input_args, pixelformat, install, parsed_pixelformat, parsed_fps, parsed_frame_dimensions);
        } 
        catch(const std::exception& e)
        {
			throw std::runtime_error((std::string("Could not open for reading.") )+e.what());
		}
        
		//std::cerr << "parsed input pixel format " << parsed_pixelformat << std::endl;
		const std::unordered_map<std::string,vidio::PixelFormat>& valid_read_pixformats = install.valid_read_pixelformats();
		fmt=valid_read_pixformats.at(pixelformat);
        native_fmt=valid_read_pixformats.at(parsed_pixelformat);
		frame_dims.width = parsed_frame_dimensions.width;
		frame_dims.height = parsed_frame_dimensions.height;
		fps = parsed_fps;
    }
	~Impl()
	{
	//	pclose(pipeout);
	//	pipeout = NULL;
	}
	
    PixelFormat fmt;
    const PixelFormat& pixelformat() const
    {
        return fmt;
    }
    PixelFormat native_fmt;
    const PixelFormat& native_pixelformat() const
    {
        return native_fmt;
    }
    double fps;
	double framerate() const
	{
        return fps;
    }

    Size frame_dims;
	Size video_frame_dimensions() const
	{
        return frame_dims;
    }
    
    size_t  video_frame_bufsize () const {
		Size sz=video_frame_dimensions();
		return (sz.width*sz.height*static_cast<size_t>(pixelformat().bits_per_pixel))/8;
	}

	bool read_video_frame(void *buf) const
	{
		unsigned int video_framebuf_sz = video_frame_bufsize();

		uint8_t* tmpbuf = static_cast<uint8_t*>(buf);
		std::size_t res;
		for(std::size_t bytes_read = 0; bytes_read < video_framebuf_sz; bytes_read += res)
		{
			res = ffmpeg_process->read_from_stdout(reinterpret_cast<char*>(tmpbuf + bytes_read), video_framebuf_sz - bytes_read);
			if(res == 0)
				return false;
		}

		return true;
    }
    bool good() const
    {
        return ffmpeg_process != nullptr;
    }
	bool read_audio_frame(void* buf) const
	{
        return false;
    }
};




Reader::Reader(const std::string& filename,const std::string& pixelformat,const FFMPEG_Install& install):
    impl(std::make_shared<Reader::Impl>(std::vector<std::string>{"-i",filename},pixelformat,install))
{}

Reader::~Reader()
{}

const PixelFormat& Reader::pixelformat() const
{
    return impl->pixelformat();
}
const PixelFormat& Reader::native_pixelformat() const
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
size_t  Reader::video_frame_bufsize () const {
    return impl->video_frame_bufsize();
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
