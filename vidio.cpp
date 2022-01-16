#include "vidio.hpp"
#include "subprocess.h"
#include<unordered_map>
#include<vector>
#include<cstdio>
#include<iostream>
#include<sstream>
#include<cstring>
#include <stdexcept>
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
    std::unordered_map<std::string,vidio::FFMPEG_Install::PixFormat>& pixfmts,
    const std::string& default_ffmpeg_path = "/usr/bin/ffmpeg")
{
	const char *const commandLine[] = {default_ffmpeg_path.c_str(), "-hide_banner","-v","24","-pix_fmts", 0};

    Subprocess ffproc(commandLine); // TODO: Fix Subprocess initialization!
	
    pixfmts.clear();
	bool line_is_a_format = false;
    std::string temps;
	while(getline_subprocess_stdout(ffproc,temps))
	{
		if(line_is_a_format) // IO... yuv420p                3            12
		{
			vidio::FFMPEG_Install::PixFormat pixfmt;
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
                pixfmt.readable =temps[1] == 'O';
                pixfmt.writable =temps[0] == 'I';
                pixfmts[pixfmt.name]=pixfmt;
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

    std::unique_ptr<Subprocess> parse_input_pixel_fmt(const std::vector<std::string>& ffmpeg_input_args,const std::string& pixelformat,const vidio::FFMPEG_Install& install, std::string& parsed_pixelformat, double& parsed_fps, vidio::Size& parsed_frame_dimensions)
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
    cmdLine.push_back("-vcodec");
    cmdLine.push_back("rawvideo");
    cmdLine.push_back("-f");
    cmdLine.push_back("rawvideo");
    if(pixelformat != "")
    {
        cmdLine.push_back("-pix_fmt");
        cmdLine.push_back(pixelformat.c_str());
    }
    
    cmdLine.push_back("-");
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
        std::cout << "Current line of open FFMPEG PROCESS: " << temps << std::endl;
		// look for format ex: Stream #0:0(und): Video: h264 (Main) (avc1 / 0x31637661), yuv420p, 1280x720 [SAR 1:1 DAR 16:9], 862 kb/s, 25 fps, 25 tbr, 12800 tbn, 50 tbc (default)
		size_t vind = temps.find("Video:");
		if((temps.find("Stream #0") != string::npos &&
			vind != string::npos))
		{
			size_t start_fmt_ind = temps.find(",", vind)+1;
			size_t end_fmt_ind = temps.find(",", start_fmt_ind);
            if ((start_fmt_ind != string::npos &&
                end_fmt_ind != string::npos))
            {
                vid_fmt = temps.substr(start_fmt_ind + 1, end_fmt_ind - (start_fmt_ind + 1));
            }
            std::stringstream ss(temps.substr(vind));
            std::string property;
            while (std::getline(ss, property, ','))
            {
                if (property.find("x") != std::string::npos)
                {
                    std::stringstream aspect_ratio_ss(property);
                    char tmp;
                    aspect_ratio_ss >> width;
                    if (width == 0)
                    {
                        continue;
                    }
                    aspect_ratio_ss >> tmp;
                    aspect_ratio_ss >> height;
                }
                if (property.find("fps") != std::string::npos)
                {
                    std::stringstream fps_ss(property);
                    fps_ss >> fps;
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

static size_t get_nchannels(std::string& layout)
{
	if(layout == "mono")
	{
		return 1;
	}
	else if(layout == "stereo")
	{
		return 2;
	}
	else
	{
		throw std::runtime_error("Vidio: Need to parse standard ffmpeg -layouts for additional layout configurations.");
	}
}

std::unique_ptr<Subprocess> parse_input_sample_fmt(const std::vector<std::string>& ffmpeg_input_args, const vidio::FFMPEG_Install& install, vidio::SampleFormat& parsed_sample_fmt, unsigned int& parsed_sample_rate)
{
	if(!install.good())
	{
		throw std::runtime_error("FFMPEG not found.");
	}

//	const char *const commandLine[] = {ffmpeg_path.c_str(),"-y","-hide_banner","-i","test.mp4","output.wav", 0};
//	const char *const commandLine[] = {ffmpeg_path.c_str(),"-y","-hide_banner","-i","sine.mp3","output.wav", 0};
	// Need to output raw audio stream:
	//const char *const commandLine[] = {ffmpeg_path.c_str(), "-y", "-hide_banner", "-i", "sine.wav", "-f", "s16le", "-c:a", "pcm_s16le", "output.raw", 0};

	std::vector<const char*> cmdLine;
    cmdLine.emplace_back(install.ffmpeg_path().c_str());
    cmdLine.emplace_back("-y"); // yes to overwrite output TODO: remove
    cmdLine.emplace_back("-hide_banner");
    
	for(const std::string& ia : ffmpeg_input_args)
    {
        cmdLine.push_back(ia.c_str());
    }
	// Set codec and data type for raw output:
    cmdLine.push_back("-f");
    cmdLine.push_back("s16le");
    cmdLine.push_back("-c:a");
    cmdLine.push_back("pcm_s16le");
    cmdLine.push_back("-");
	cmdLine.push_back(0);
    std::unique_ptr<Subprocess> ffmpeg_proc=std::make_unique<Subprocess>(cmdLine.data());
	
	std::string temps, sample_codec, sample_layout, sample_data_type;
	unsigned int sample_rate = 0;
	while(sample_rate == 0)
	{
		if(!getline_subprocess_stderr(*ffmpeg_proc,temps))
		{
			break; // end of err stream.
		}

		size_t audio_ind = temps.find("Audio: ");
		if(audio_ind != std::string::npos)// Stream #0:0: Audio: pcm_s16le ([1][0][0][0] / 0x0001), 44100 Hz, mono, s16, 705 kb/s
		{
			size_t sample_codec_begin_ind = audio_ind + 7; // "Audio: " is 7 characters: skip the string.
			size_t sample_codec_end_ind = temps.find(' ', sample_codec_begin_ind + 1); // Audio: pcm_s16le (...
			if(sample_codec_end_ind != std::string::npos)
			{
				sample_codec = temps.substr(sample_codec_begin_ind, sample_codec_end_ind-sample_codec_begin_ind);
			}
			size_t hz_ind = temps.find("Hz");
			if(hz_ind != std::string::npos)
			{
				size_t comma_before_hz_ind = temps.rfind(',', hz_ind) + 2; // add 2 characters for ", "
				if(comma_before_hz_ind != std::string::npos)
				{
					sample_rate = static_cast<unsigned int>(stoi(temps.substr(comma_before_hz_ind, hz_ind - comma_before_hz_ind)));
				}
				// "Hz, " is 4 characters: skip the string to next layout param: Hz, mono, ...
				size_t sample_layout_begin_ind = hz_ind + 4;
				size_t sample_layout_end_ind = temps.find(',', sample_layout_begin_ind);
				if(sample_layout_end_ind != std::string::npos)
				{
					sample_layout = temps.substr(sample_layout_begin_ind, sample_layout_end_ind - sample_layout_begin_ind);
					size_t sample_data_type_begin_ind = sample_layout_end_ind + 2;
					size_t sample_data_type_end_ind = temps.find(',', sample_data_type_begin_ind);
					if(sample_data_type_end_ind != std::string::npos)
					{
						sample_data_type = temps.substr(sample_data_type_begin_ind, sample_data_type_end_ind - sample_data_type_begin_ind);
						//cout << "Input Audio Format: " << "sample codec: " << sample_codec << " sample rate: " << sample_rate << "Hz sample_layout: " << sample_layout << " sample data type: " << sample_data_type << endl;
						parsed_sample_fmt = vidio::SampleFormat(sample_codec, sample_layout, get_nchannels(sample_layout), sample_data_type);
						parsed_sample_rate = sample_rate;
					}
				}
			}
			break;
		}
	}
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
	std::unordered_map<std::string,vidio::FFMPEG_Install::PixFormat> pixfmts;
    Impl(const std::vector<std::string>& additional_search_locations={})
    {
        // call pix_fmts with default install location and if it doesn't work, test additional_search_locations
        std::vector<std::string> platform_defaults = {"ffmpeg.exe", "/usr/bin/ffmpeg"}; // remove ffmpeg.exe if not in windows.
        std::string ffmpeg_path = "";
        platform_defaults.insert(platform_defaults.end(),additional_search_locations.begin(),additional_search_locations.end());
        m_good = false;
        
        std::unordered_map<std::string,vidio::FFMPEG_Install::PixFormat> tpixformats;
		vidio::SampleFormat tsample_fmt;
		unsigned int tsample_rate;
        for(
            std::vector<std::string>::const_iterator search_location = platform_defaults.begin(); 
        !m_good && search_location != platform_defaults.end(); search_location++)
        {	
            m_good = parse_ffmpeg_pixfmts(tpixformats, *search_location);
            if(m_good)
            {
                str_ffmpeg_path = *search_location;
            }
        }
        if(m_good)
        {
            pixfmts=tpixformats;
        }
        else
        {
            throw std::runtime_error("Unable to find ffmpeg binary");
        }
	}
};


const std::unordered_map<std::string,vidio::FFMPEG_Install::PixFormat>&  FFMPEG_Install::pixformats() const
{
    return impl->pixfmts;
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
    std::unique_ptr<Subprocess> ffmpeg_process_audio;
    Impl(const std::vector<std::string>& ffmpeg_input_args, const std::string& requested_pixelformat,const FFMPEG_Install& install)
    {
		if(!install.good())
		{
			throw std::runtime_error("FFMPEG Installation not found.");
		}
		std::string native_pixelformat = "";
        std::string actual_pixelformat = requested_pixelformat;
        
        const std::unordered_map<std::string,vidio::FFMPEG_Install::PixFormat>& pixformats = install.pixformats();
        
        double parsed_fps = -1;
		vidio::Size parsed_frame_dimensions;
		vidio::SampleFormat parsed_sample_fmt;
		unsigned int parsed_sample_rate = 0;
        try
        {
			std::string input_filename_extension = ffmpeg_input_args[1].substr(ffmpeg_input_args[1].find_last_of("."));
			bool do_parse_audio = (input_filename_extension == ".wav") || (input_filename_extension == ".mp3"); // TODO: detect from input file once integrated.
			if(do_parse_audio)
			{
				ffmpeg_process_audio=parse_input_sample_fmt(ffmpeg_input_args, install, parsed_sample_fmt, parsed_sample_rate);
				sample_fmt = parsed_sample_fmt;
				sample_rate = parsed_sample_rate;
				return;// TODO: skipping video init with audio case for now.
			}
			ffmpeg_process=parse_input_pixel_fmt(ffmpeg_input_args, actual_pixelformat, install, native_pixelformat, parsed_fps, parsed_frame_dimensions);
        } 
        catch(const std::exception& e)
        {
			throw std::runtime_error((std::string("Could not open for reading."))+e.what());
		}
        if(requested_pixelformat == "")
        {
            actual_pixelformat=native_pixelformat;
        }
        
        auto rnative=pixformats.find(native_pixelformat);
        native_fmt={"0",0,0};
        if(rnative == pixformats.end())
        {
            if(actual_pixelformat == native_pixelformat) {
                throw std::runtime_error(std::string("Could not find metatdata for raw input pixelformat '")+actual_pixelformat+"'");
            }
        }
        else
        {
            native_fmt=rnative->second;
        }
        
        if(actual_pixelformat==native_pixelformat)
        {
            fmt=native_fmt;
        }
        else
        {
            auto rout=pixformats.find(actual_pixelformat);
            if(rout == pixformats.end()){
                throw std::runtime_error(std::string("Pixelformat is not a valid read pixelformat for this ffmpeg '")+actual_pixelformat+"'");
            }
            fmt=rout->second;
        }
		frame_dims.width = parsed_frame_dimensions.width;
		frame_dims.height = parsed_frame_dimensions.height;
		fps = parsed_fps;
		sample_fmt = parsed_sample_fmt;
		sample_rate = parsed_sample_rate;
    }
	~Impl()
	{
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
    SampleFormat sample_fmt;
    const SampleFormat& sampleformat() const
    {
        return sample_fmt;
    }
	unsigned int sample_rate;
	unsigned int samplerate() const
	{
		return sample_rate;
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
			res = ffmpeg_process->read_from_stdout(reinterpret_cast<char*>(tmpbuf) + bytes_read, video_framebuf_sz - bytes_read);
			if(res == 0)
				return false;
		}

		return true;
    }
    bool good() const
    {
        return ffmpeg_process != nullptr;
    }

	bool read_audio_samples(void* buf, const size_t& nsamples) const
	{
		const size_t sample_bytes_sz = sample_fmt.nbytes * sample_fmt.nchannels;
		const size_t audio_samplebuf_bytes_sz = sample_bytes_sz * nsamples;
		uint8_t* tmpbuf = static_cast<uint8_t*>(buf);
		std::size_t res;
		for(std::size_t bytes_read = 0; bytes_read < audio_samplebuf_bytes_sz; bytes_read += res)
		{
			res = ffmpeg_process_audio->read_from_stdout(reinterpret_cast<char*>(tmpbuf) + bytes_read, audio_samplebuf_bytes_sz - bytes_read);
			if(res == 0)
				return false;
		}

		return true;
    }
};

std::size_t SampleFormat::get_sample_nbytes(const std::string& sample_data_type) const
{
	std::string _data_type = sample_data_type;
	// Remove endianness from data type number of bytes calculation:
	if((sample_data_type.find("le") != std::string::npos) || (sample_data_type.find("be") != std::string::npos))
	{
		_data_type = sample_data_type.substr(0,sample_data_type.size() - 2);
	}

	if(_data_type == "u8" || _data_type == "u8p")
		return sizeof(uint8_t);
	else if(_data_type == "s16" || _data_type == "s16p")
		return sizeof(int16_t);
	else if(_data_type == "s32" || _data_type == "s32p")
		return sizeof(int32_t);
	else if(_data_type == "flt" || _data_type == "fltp")
		return sizeof(float);
	else if(_data_type == "dbl" || _data_type == "dblp")
		return sizeof(double);
	else if(_data_type == "s64" || _data_type == "s64p")
		return sizeof(int64_t);
	else
		throw std::runtime_error("Vidio: invalid sample fmt.  Must be valid from ffmpeg -sample_fmts.");
}

Reader::Reader(const std::vector<std::string>& ffmpeg_input_args,const std::string& pixelformat,const FFMPEG_Install& install):
    impl(std::make_shared<Reader::Impl>(ffmpeg_input_args,pixelformat,install))
{}
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
const SampleFormat& Reader::sampleformat() const
{
    return impl->sampleformat();
}
double Reader::framerate() const
{
    return impl->framerate();
}
unsigned int Reader::samplerate() const
{
    return impl->samplerate();
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
bool Reader::read_audio_samples(void* buf, const size_t& nsamples) const
{
    return impl->read_audio_samples(buf, nsamples);
}

std::unique_ptr<Subprocess> start_ffmpeg_output_audio(const std::vector<std::string>& encode_ffmpeg_params, const vidio::FFMPEG_Install& install, const vidio::SampleFormat& sample_fmt, const unsigned int& sample_rate, const std::string& filename)
{
	if(!install.good())
	{
		throw std::runtime_error("FFMPEG not found.");
	}

	std::vector<const char*> cmdLine;
    cmdLine.emplace_back(install.ffmpeg_path().c_str());
    cmdLine.emplace_back("-hide_banner");
    cmdLine.emplace_back("-y"); // yes to overwrite output TODO: remove
	if(!((encode_ffmpeg_params.size() == 1) && (encode_ffmpeg_params[0] == "")))
	{
		for(const std::string& ia : encode_ffmpeg_params)
		{
		    cmdLine.push_back(ia.c_str());
		}
	}
	// Set codec and data type for raw output:
    cmdLine.push_back("-f");
    cmdLine.push_back(sample_fmt.data_type.c_str());
    cmdLine.push_back("-c:a");
    cmdLine.push_back(sample_fmt.codec.c_str());
	cmdLine.push_back("-ar");
	std::stringstream ss;
	ss << sample_rate;
	cmdLine.push_back(ss.str().c_str());
	cmdLine.push_back("-i");
	cmdLine.push_back("-"); // equivalent arg to "-" is "pipe:" // read from stdin, where we're writing to.
	cmdLine.push_back(filename.c_str());
	cmdLine.push_back(0);
    std::unique_ptr<Subprocess> ffmpeg_proc=std::make_unique<Subprocess>(cmdLine.data(), Subprocess::JOIN);
	return ffmpeg_proc;
}

class Writer::Impl
{
public:
    std::unique_ptr<Subprocess> ffmpeg_process_audio;
    Impl(const std::vector<std::string>& encode_ffmpeg_args, const Size& size, double framerate, const std::string& pixelfmt="rgb24", const FFMPEG_Install& install=FFMPEG_Install(), const std::string& filename="")
    {
		// Raw audio defaults:
		SampleFormat requested_sample_fmt("pcm_s16le", "mono", 1, "s16le");
		unsigned int requested_sample_rate = 44100;
		sample_fmt = requested_sample_fmt;
		sample_rate = requested_sample_rate;
		if(!install.good())
		{
			throw std::runtime_error("FFMPEG Installation not found.");
		}

		try
		{
			std::string output_filename_extension = filename.substr(filename.find_last_of("."));
			bool do_write_audio = (output_filename_extension == ".wav") || (output_filename_extension == ".mp3"); // TODO: detect from input file once integrated.
			if(do_write_audio)
			{
				ffmpeg_process_audio=start_ffmpeg_output_audio(encode_ffmpeg_args, install, requested_sample_fmt, requested_sample_rate, filename);
				if(ffmpeg_process_audio == nullptr)
				{
					throw std::runtime_error("Error starting audio writer.");
				}
			}
        } 
        catch(const std::exception& e)
        {
			throw std::runtime_error((std::string("Could not open for writing."))+e.what());
		}
    }
	~Impl()
	{
		if((ffmpeg_process_audio != nullptr) && (ffmpeg_process_audio->input != nullptr))
		{
			fflush(ffmpeg_process_audio->input);
		}
	}

    SampleFormat sample_fmt;
    const SampleFormat& sampleformat() const
    {
        return sample_fmt;
    }
	unsigned int sample_rate;
	unsigned int samplerate() const
	{
		return sample_rate;
	}
    bool good() const
    {
        return ffmpeg_process_audio != nullptr;
    }

	bool write_audio_samples(const void* buf, const size_t& nsamples) const
	{
		size_t nchannels = sampleformat().nchannels;
		size_t audio_samplebuf_size = sampleformat().nbytes * nsamples * nchannels;
		const uint8_t* tmpbuf = static_cast<const uint8_t*>(buf);
		std::size_t res;
		for(std::size_t bytes_read = 0; bytes_read < audio_samplebuf_size; bytes_read += res)
		{
			res = ffmpeg_process_audio->write_to_stdin(reinterpret_cast<const char*>(tmpbuf + bytes_read), audio_samplebuf_size - bytes_read);
			fflush(ffmpeg_process_audio->input);
			if(res == 0)
				return false;
		}
		return true;
    }
};

Writer::Writer(const std::string& filename,
		const Size& size,
		double framerate,
		const std::string& pixelfmt,
		const std::string& encode_ffmpeg_params,
		const FFMPEG_Install& install):
	impl(std::make_shared<Writer::Impl>(std::vector<std::string>{encode_ffmpeg_params},size,framerate,pixelfmt,install,filename))
{
}

Writer::~Writer()
{}

const SampleFormat& Writer::sampleformat() const
{
    return impl->sampleformat();
}
unsigned int Writer::samplerate() const
{
    return impl->samplerate();
}

bool Writer::write_audio_samples(const void* buf, const size_t& nsamples) const
{
	return impl->write_audio_samples(buf, nsamples);
}

}
