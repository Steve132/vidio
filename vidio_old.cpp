#include "vidio.hpp"
#include "vidio_internal.hpp"
#include <list>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <unordered_map>
#include <iostream>

static std::string get_ffmpeg_prefix(const std::string& usr_override="")
{
	std::list<std::string> possible_locations;
	if(usr_override != "")
	{
		possible_locations.push_back(usr_override);
	}
	
	const std::vector<std::string>& pform=vidio::platform::get_default_ffmpeg_search_locations();
	copy(pform.cbegin(),pform.cend(),std::back_inserter(possible_locations));
	
	for(auto pl:possible_locations)
	{
		try
		{
			vidio::platform::create_process_reader_stream(pl+" -loglevel panic -hide_banner");
			return pl;
		}
		catch(const std::exception& e)
		{}
	}
	throw std::runtime_error("No executable ffmpeg process found");
}

inline static bool compute_bigendian()
{
	union a
	{
		uint16_t s;
		const uint8_t ub[2];
		a():s(0x0001)
		{}
	} ai;
	return ai.ub[0]!=0x01;
}
inline static bool is_bigendian()
{
	static const bool test=compute_bigendian();
	return test;
}
static std::string get_fmt_code(const std::uint32_t& typesize,const uint32_t num_channels)
{
	const std::string types_le[2][4]={
		{"gray8","ya8","rgb24","rgba"},
		{"gray16","ya16","rgb48","rgba64"}
	};         //AV_PIX_FMT_RGBA64BE_LIBAV or AV_PIX_FMT_RGBA64BE?  try both

	if(typesize > 2 || typesize < 1)
	{
		throw std::range_error("The typesize must be 1 or 2 (8-bit or 16-bit color ONLY)");
	}		
	if(num_channels > 4 || num_channels < 1)
	{
		throw std::range_error("There must be at least 1 channel and less than 5 channels");
	}
	std::string typesel=types_le[typesize-1][num_channels-1];
	if(typesize > 1)
	{
		if(is_bigendian())
		{
			typesel+="be";
		}
		else
		{
			typesel+="le";
		}
	}
	return typesel;
}


namespace vidio
{

namespace priv
{
class StreamImpl
{
public:
	std::string ffmpeg_path;
		
	Size size;
	uint32_t channels;
	uint32_t typewidth;
	double framerate;
	
	bool is_open;
	
	StreamImpl(
		const Size& tsize,
		const uint32_t& tchannels,
		const uint32_t& ttypewidth,
		const double& tframerate,
		const std::string& search_path_override):
		size(tsize),
		channels(tchannels),
		typewidth(ttypewidth),
		framerate(tframerate)
	{
		ffmpeg_path=get_ffmpeg_prefix(search_path_override);
	}
	
	virtual ~StreamImpl()
	{}
};

struct pfmtpair
{
	uint32_t channels,typewidth;
};
typedef std::pair<std::string,pfmtpair> pfmttriple;

std::ostream& operator<<(std::ostream& out,const pfmtpair& pdata)
{
	return out << pdata.channels << '.' << pdata.typewidth;
}

std::istream& operator>>(std::istream& ins,pfmttriple& pdata)
{
	std::string tmp;
	ins >> tmp >> pdata.first >> pdata.second.channels >> pdata.second.typewidth;
	
	if(pdata.second.channels != 0)
	{
		uint32_t realtypewidth=pdata.second.typewidth / pdata.second.channels;
		pdata.second.typewidth=1;
		if(realtypewidth >= 16)
		{
			pdata.second.typewidth=2;
			return ins;
		}
		std::string pstr("p");
		for(int i=9;i<=16;i++)
		{
			std::string pistr=pstr+std::to_string(i);
			if(pdata.first.find(pistr+"be") != std::string::npos || pdata.first.find(pistr+"le") != std::string::npos)
			{
				pdata.second.typewidth=2;
				return ins;
			}
		}	
	}
	return ins;
}



typedef std::unordered_map<std::string,pfmtpair> pfmtmap_type;

pfmtmap_type load_pfmt_map(const std::string& ffpath)
{
	std::ostringstream cmd;
	cmd << ffpath << " -v panic -pix_fmts";
	std::shared_ptr<std::istream> pstreamptr=vidio::platform::create_process_reader_stream(cmd.str());

	std::string tmp;
	int dashcount;
	for(int dashcount=0;*pstreamptr && dashcount < 5;)
	{
		int hscan=pstreamptr->get();
		if(hscan == '-')
		{
			dashcount++;
		}
		else
		{
			dashcount=0;
		}
	}
	return pfmtmap_type((std::istream_iterator<pfmttriple>(*pstreamptr)),std::istream_iterator<pfmttriple>());
}

struct ffprobe_info
{
	Size size;
	uint32_t channels;
	uint32_t typewidth;
	double framerate;
	uint32_t num_frames;
	
	ffprobe_info(std::string ffpath,const std::string& filename)
	{
		static pfmtmap_type pfmtmap=load_pfmt_map(ffpath);
		std::cout << "NUMKEYS:" << pfmtmap.size() << std::endl;
		for(auto fpair : pfmtmap)
		{
			std::cout << fpair.first << ":" << fpair.second << std::endl;
		}
		//platform::create_process_reader_streambuf(ffpath);
		
		std::ostringstream cmd;
		cmd << ffpath << " -v panic -select_streams V:0 -show_entries stream=width,height,pix_fmt,duration,nb_frames -of default=noprint_wrappers=1:nokey=1 -i " << filename;
		
		std::shared_ptr<std::istream> pstreamptr=vidio::platform::create_process_reader_stream(cmd.str());
		std::istream& pstream=*pstreamptr;
		
		pstream >> size.width >> size.height;
		
		std::string pix_fmt;
		pstream >> pix_fmt;
		pfmtpair pair=pfmtmap[pix_fmt];
		channels=pair.channels;
		typewidth=pair.typewidth;
		
		double duration;
		pstream >> duration >> num_frames;
		framerate=duration/static_cast<double>(num_frames);
	}
};

class ReaderImpl: public StreamImpl
{
public:
	std::shared_ptr<std::istream> pstreamptr;
	uint32_t num_frames;
	ReaderImpl(
		const std::string& filename,
		const Size& tsize,
		const uint32_t tchannels,
		const uint32_t ttypewidth,
		const double tframerate,
		const std::string& extra_decode_ffmpeg_params,
		const std::string& search_path_override):
		StreamImpl(tsize,tchannels,ttypewidth,tframerate,search_path_override)
	{
		std::ostringstream cmdin;
		std::ostringstream cmdout;
		std::ostringstream cmd;
		
		std::string ffprobe_path=ffmpeg_path;
		auto ffb=ffprobe_path.rbegin();
		*(ffb++)='b';*(ffb++)='o';*(ffb++)='r';*(ffb++)='p';
		ffprobe_path.push_back('e');
		ffprobe_info info(ffprobe_path,filename);
		//Use :v:0 as the stream specifier for all options

		//Use and -an -sn to disable other streams.
		cmdout << " -an -sn"; 

		//cmdin << " -loglevel trace -hide_banner";
		cmdin << " -loglevel fatal -hide_banner";
		
		//If size is not specified (0), do populate size datatype with probed info.
		if(size.width==0 || size.height==0)
		{
			//size=populate from ffprobe 
		}
		//otherwise, if its' specified, add a scaling filter tothe filterchain on output.
		else
		{
			cmdout << " -s:v:0 " << size.width << "x" << size.height;
		}
		
		if(channels==0)
		{
			//channels=populate from ffprobe (probably do all the ffprobe populates at once if any of the info is unknown)
		}
		
		if(typewidth==0)
		{
			//typewidth=populate from ffprobe
		}
		
		if(framerate < 0.0)
		{
			//framerate=populate from ffprobe
		}
		else
		{
			//cmdin << " -framerate:v:0 " << framerate;
			cmdout << " -r:v:0 " << framerate;
		}
		
		cmdout << " -c:v:0 rawvideo -f rawvideo -pix_fmt " << get_fmt_code(typewidth,channels);
		cmdin << " " << extra_decode_ffmpeg_params;
		cmd << ffmpeg_path << cmdin.str() << " -i " << filename << cmdout.str() << " - ";
		
		pstreamptr=vidio::platform::create_process_reader_stream(cmd.str());
	}	
};

class WriterImpl: public StreamImpl
{
public:
	std::shared_ptr<std::ostream> pstreamptr;
	WriterImpl(
		const std::string& filename,
		const Size& tsize,
		const uint32_t tchannels,
		const uint32_t ttypewidth,
		const double tframerate,
		const std::string& extra_encode_ffmpeg_params,
		const std::string& search_path_override):
		StreamImpl(tsize,tchannels,ttypewidth,tframerate,search_path_override)
	{
		std::ostringstream cmdin;
		std::ostringstream cmdout;
		std::ostringstream cmd;
		
		cmdin << " -f rawvideo -pixel_format " << get_fmt_code(typewidth,channels);
		cmdin << " -framerate " << framerate;
		cmdin << " -video_size " << size.width << "x" << size.height;
		
		cmdout << " " << extra_encode_ffmpeg_params;
		cmd << ffmpeg_path << cmdin.str() << " -i - " << cmdout.str() << " " << filename;
		
		pstreamptr=vidio::platform::create_process_writer_stream(cmd.str());
	}
};

}



const Size Size::Unknown=Size(0,0);

Stream::Stream(priv::StreamImpl* iptr):
	impl(iptr),
	size(iptr->size),
	channels(iptr->channels),
	typewidth(iptr->typewidth),
	framerate(iptr->framerate),
	is_open(iptr->is_open),
	frame_buffer_size(size.width*size.height*channels*typewidth)
{}


Stream::~Stream()
{}


Reader::Reader(	const std::string& filename,
		const Size& tsize,
		const uint32_t tchannels,
		const uint32_t ttypewidth,
		const double tframerate,
		const std::string& extra_decode_ffmpeg_params,
		const std::string& search_path_override):
		
		Stream(	new priv::ReaderImpl(
			filename,
			tsize,
			tchannels,
			ttypewidth,
			tframerate,
			extra_decode_ffmpeg_params,
			search_path_override)
		),
		framesinstream(dynamic_cast<priv::ReaderImpl*>(impl.get())->pstreamptr),
		num_frames(dynamic_cast<const priv::ReaderImpl*>(impl.get())->num_frames)
{}

Writer::Writer(const std::string& filename,
		const Size& tsize,
		const uint32_t tchannels,
		const uint32_t ttypewidth,
		const double tframerate,
		const std::string& extra_encode_ffmpeg_params,
		const std::string& search_path_override):
		
		Stream(new priv::WriterImpl(
			filename,
			tsize,
			tchannels,
			ttypewidth,
			tframerate,
			extra_encode_ffmpeg_params,
			search_path_override)),
		framesoutstream(dynamic_cast<priv::WriterImpl*>(impl.get())->pstreamptr)
{}

}


	

	
	//eventually use HW formats. https://www.ffmpeg.org/doxygen/2.5/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5
