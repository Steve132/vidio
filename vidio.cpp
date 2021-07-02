#include "vidio.hpp"
#include "subprocess.h"
#include<unordered_map>
#include<vector>

class FFMPEG_Install
{
public:
	FFMPEG_Install(const std::vector<std::string>& additional_search_locations={});
	bool good() const {return m_good;}

	bool m_good;
	std::string ffmpeg_path;
	std::vector<vidio::PixelFormat> input_pixel_formats;
	std::vector<vidio::PixelFormat> output_pixel_formats;
	std::unordered_map<std::string,vidio::PixelFormat> readable_formats; //"O"
	std::unordered_map<std::string,vidio::PixelFormat> writeable_formats; //"I"

	static const FFMPEG_Install& instance(const std::vector<std::string>& additional_search_locations={})
	{
		static std::shared_ptr<FFMPEG_Install> def;
		if(!def || !def->good()) { def=std::make_shared<FFMPEG_Install>(additional_search_locations); }
		if(!def || !def->good()) {
			throw std::runtime_error("Error, can't find ffmpeg.");
		}
		return *def;
	}
	
    bool is_good;
};

//the default loglevel should 

bool parse_ffmpeg_pixfmts(
    std::vector<vidio::PixelFormat>& input_pixel_formats, 
    std::vector<vidio::PixelFormat>& output_pixel_formats, 
    const std::string& default_ffmpeg_path = "/usr/bin/ffmpeg")
{
	const char *const commandLine[] = {default_ffmpeg_path.c_str(), "-v","24","-pix_fmts", 0};
	struct subprocess_s process;
	int ret = -1;
	FILE *stdout_file;
	int nchars = 128;
	char temp[nchars];

	if(subprocess_create(commandLine, 0, &process) != 0)
		return false;
	stdout_file = subprocess_stdout(&process); // note: not stderr! pix_fmts goes to stdout

	input_pixel_formats.clear();
	output_pixel_formats.clear();
	bool line_is_a_format = false;
	while(!feof(stdout_file))
	{
		fgets(temp, nchars, stdout_file);
		std::string temps(temp);
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
					input_pixel_formats.push_back(pixfmt);
				}
				if(temps[1] == 'O')
				{
					output_pixel_formats.push_back(pixfmt);
				}
			}
		}
		if(temps.find("FLAGS") != std::string::npos)
		{
			fgets(temp, nchars, stdout_file); // ignore next line.
			line_is_a_format = true;
		}
	}
	subprocess_join(&process, &ret);
	if(ret != 0)
	{
		return false;
	}
	subprocess_destroy(&process);
	return true;
}

FFMPEG_Install::FFMPEG_Install(const std::vector<std::string>& additional_search_locations)
{
	// call pix_fmts with default install location and if it doesn't work, test additional_search_locations
	std::vector<std::string> platform_defaults = {"/usr/bin/ffmpeg"};
	std::string ffmpeg_path = "";
	platform_defaults.insert(platform_defaults.end(),additional_search_locations.begin(),additional_search_locations.end());
	m_good = false;
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
		for(const vidio::PixelFormat& pf:input_pixel_formats)
		{
			writeable_formats[pf.name] = pf;
		}
		for(const vidio::PixelFormat& pf:output_pixel_formats)
		{
			readable_formats[pf.name] = pf;
		}
	}
}

//eventually use HW formats. https://www.ffmpeg.org/doxygen/2.5/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5

namespace vidio
{

class Reader::Impl
{
public:
    Impl(const std::string& filename,const std::string& pixelformat,const std::vector<std::string>& extra_ffmpeg_locations)
    {
        filepointer=NULL;
        //TODO open filepointer here.  parse fps and pixelformat string. 
        if(pixelformat == "") { 
            pixelformat=parsed_pixelformat;
        }
        fmt=FFMPEG_Install(
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
        
    }
	bool read_audio_frame(void* buf) const
	{
    }
};


Reader::Reader(const std::string& filename,const std::string& pixelformat,const std::vector<std::string>& extra_ffmpeg_locations):
    impl(std::make_shared<Reader::Impl>(filename,pixelformat,extra_ffmpeg_locations))
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
std::vector<PixelFormat> Reader::valid_pixelformats(const std::vector<std::string>& additional_search_locations)
{
    return FFMPEG_Install::instance(additional_search_locations).output_pixel_formats;
}















class Writer::Impl
{
public:

};

std::vector<PixelFormat> Writer::valid_pixelformats(const std::vector<std::string>& additional_search_locations)
{
    return FFMPEG_Install::instance(additional_search_locations).input_pixel_formats;
}

}
