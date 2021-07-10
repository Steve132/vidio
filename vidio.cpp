#include "vidio.hpp"
#include "subprocess.h"
#include<unordered_map>
#include<vector>
#include<cstdio>
#include<iostream>
#include<sstream>
using namespace std;

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

    /*Subprocess ffproc(commandLine); // TODO: Fix Subprocess initialization!
	
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
	*/
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
		string temps(temp);
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
			// TODO: Replace once Subprocess class fixed: std::getline(ffproc.outstream,temps); // ignore next line.
			fgets(temp, nchars, stdout_file); // ignore next line.
			line_is_a_format = true;
		}
	}
	// TODO: Replace once Subprocess class fixed:	return ffproc.join()==0;
	subprocess_join(&process, &ret);
	if(ret != 0)
	{
		return false;
	}
	subprocess_destroy(&process);
	return true;
}

bool parse_input_pixel_fmt(const std::string& filename,const std::string& pixelformat,const vidio::FFMPEG_Install& install, std::string& parsed_pixelformat, double& parsed_fps, vidio::Size& parsed_frame_dimensions)
{
	if(!install.good())
	{
		return false;
	}
	// TODO: replace rgba with pixelformat specified by input parameter
	const char *const commandLine[] = {
			install.ffmpeg_path().c_str(), "-i", filename.c_str(), "-pix_fmt", "rgba", "-vcodec", "rawvideo", "-f", "image2pipe", "pipe:1", 0};
	struct subprocess_s process;
	int ret = -1;
	FILE *stderr_file, *stdout_file;
	int nchars = 128;
	char temp[nchars];

	if(subprocess_create(commandLine, 0, &process) != 0)
		return false;
  // Note: stderr is where the pixfmt for the input is written to, but reading from stderr disables reading from stdout which contains the frame data.  The subprocess_option_combined_stdout_stderr appears to not work with ffmpeg.
	stderr_file = subprocess_stderr(&process);

	int width = -1, height = -1;
	double fps = -1;
	string vid_fmt = "";
	while(!feof(stderr_file) && (fps == -1))
	{
		fgets(temp, nchars, stderr_file);
		string temps(temp);
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
		}
	}
	cerr << "Detected video format for: " << filename << ": " << vid_fmt << " " << fps << "fps " << width << "x" << height << endl;
	parsed_pixelformat = vid_fmt;
	parsed_fps = fps;
	parsed_frame_dimensions.width = width;
	parsed_frame_dimensions.height = height;
	subprocess_destroy(&process);
	return true;
}

namespace vidio
{

class FFMPEG_Install::Impl
{
public:
	
	bool good() const {return m_good;}
	const std::string ffmpeg_path() const {return str_ffmpeg_path;}

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

const std::string FFMPEG_Install::ffmpeg_path() const
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
    Impl(const std::string& filename,const std::string& pixelformat,const FFMPEG_Install& install)
    {
		if(!install.good())
		{
			throw std::runtime_error("FFMPEG Installation not found.");
		}
		std::string parsed_pixelformat = "";
		double parsed_fps = -1;
		vidio::Size parsed_frame_dimensions;
		if(!parse_input_pixel_fmt(filename, pixelformat, install, parsed_pixelformat, parsed_fps, parsed_frame_dimensions)) // TODO: Update for Subprocess class once fixed: open only one ffmpeg call: integrate reading stderr for pixformat with same ffmpeg call as opening for reading once Subprocess class fixed.
		{
			throw std::runtime_error("Could not open for reading." + filename);
		}

		std::unordered_map<std::string,vidio::PixelFormat> valid_read_pixformats = install.valid_read_pixelformats();
		std::string raw_pixformat = "rgba";
        fmt=valid_read_pixformats[raw_pixformat]; //parsed_pixelformat]; // TODO: Update for parsed_pixelformat or input pixelformat rather than rgba!
		frame_dims.width = parsed_frame_dimensions.width;
		frame_dims.height = parsed_frame_dimensions.height;
		fps = parsed_fps;

		// TODO: replace rgba with pixelformat specified by input parameter
		const char *const commandLine[] = {
				install.ffmpeg_path().c_str(), "-i", filename.c_str(), "-pix_fmt", "rgba", "-vcodec", "rawvideo", "-f", "image2pipe", "pipe:1", 0};
		if(subprocess_create(commandLine, 0, &process) != 0)
		{
			throw std::runtime_error("Could not open for reading." + filename);
		}
        filepointer = subprocess_stdout(&process); // TODO: update for Subprocess class once fixed.
		// TEST ONLY: Write stream to verify read
		std::stringstream ss;
		ss << "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt " << "rgba" << " -s ";
		ss << frame_dims.width << "x" << frame_dims.height;
		ss << " -r " << static_cast<int>(fps) << " -i - -f mp4 -q:v 5 -an -vcodec mpeg4 out.mp4";
		cerr << "Open for writing:" << ss.str() << endl;
		pipeout = popen(ss.str().c_str(), "w");
    }
	~Impl()
	{
		if(pipeout != NULL) // TODO: debug only, remove later.
		{
			fflush(pipeout);
			pclose(pipeout);
		}
		subprocess_destroy(&process); // TODO: remove on Subprocess class once fixed.
	}
	struct subprocess_s process; // TODO: update for Subprocess class once fixed.
    FILE* filepointer;
	FILE* pipeout; // TODO: debug only, remove later.
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
		unsigned int video_framebuf_sz = frame_dims.width*frame_dims.height*(fmt.bits_per_pixel/8);
		if(!feof(filepointer) && !ferror(filepointer))
		{
			std::size_t res = fread( buf, video_framebuf_sz, 1, filepointer );
			fflush( filepointer );
			if(res > 0)
			{
				if( pipeout ) // TODO: debug only, remove later.
				{
					fwrite(buf, sizeof(unsigned char), video_framebuf_sz, pipeout);
					fflush( pipeout );
				}
			}
			return true;
		}
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

Reader::~Reader()
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
