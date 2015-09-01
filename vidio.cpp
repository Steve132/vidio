#include "vidio.hpp"
#include "vidio_internal.hpp"
#include <list>
#include <algorithm>
#include <iterator>
#include <sstream>

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
			vidio::platform::create_process_reader_streambuf(pl);
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
		throw std::range_error("There must be at least 1 channel and less than 4 channels");
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
	
	std::unique_ptr<std::streambuf> pstreambuf;
	
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

class ReaderImpl: public StreamImpl
{
public:
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
		//Use :v:0 as the stream specifier for all options

		//Use and -an -sn to disable other streams.
		cmdout << " -an -sn"; 
		
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
		cmd << ffmpeg_path << cmdin.str() << " -i " << filename << cmdout.str() << "-";
		
		pstreambuf.reset(vidio::platform::create_process_reader_streambuf(cmd.str()));
	}	
};

class WriterImpl: public StreamImpl
{
public:
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
		cmd << ffmpeg_path << cmdin.str() << " -i -" << cmdout.str() << " " << filename;
		
		pstreambuf.reset(vidio::platform::create_process_writer_streambuf(cmd.str()));
	}
};

}



Stream::Stream(priv::StreamImpl* iptr):
	impl(iptr),
	sbuf(iptr->pstreambuf.get()),
	size(iptr->size),
	channels(iptr->channels),
	typewidth(iptr->typewidth),
	framerate(iptr->framerate),
	is_open(iptr->is_open),
	frame_size_bytes(size.width*size.height*channels*typewidth)
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
		framesinstream(impl->pstreambuf.get()),
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
		framesoutstream(impl->pstreambuf.get())
{}

}


	

	
	//eventually use HW formats. https://www.ffmpeg.org/doxygen/2.5/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5