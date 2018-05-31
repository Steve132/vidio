#ifndef VIDIO_HPP
#define VIDIO_HPP

#include<memory>
#include<iostream>
#include<cstdint>

namespace vidio
{
namespace priv
{
class StreamImpl;
class ReaderImpl;
class WriterImpl;
}

struct Size
{
public:
	uint32_t width,height;
	Size(uint32_t w=0,uint32_t h=0):
		width(w),height(h)
	{}
	static const Size Unknown;
};

class Stream
{
protected:
	std::unique_ptr<priv::StreamImpl> impl;
public:
	const Size size;
	const uint32_t channels;
	const uint32_t typewidth;
	const double framerate;
	const bool is_open;

	const size_t frame_buffer_size;
	
	Stream(priv::StreamImpl* iptr);
	virtual ~Stream();
};

class Reader: public Stream
{
protected:
	std::shared_ptr<std::istream> framesinstream; 
public:
	const uint32_t num_frames; //How do we do this?
	
	Reader(	const std::string& filename,
		const Size& tsize=Size(),
		const uint32_t tchannels=0,
		const uint32_t ttypewidth=0,
		const double tframerate=-1.0,
		const std::string& extra_decode_ffmpeg_params="",
		const std::string& search_path_override="");
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory.
	bool read(void* buf,size_t num_frames=1)
	{
		return (bool)framesinstream->read((char*)buf,num_frames*frame_buffer_size);
	}
	operator bool() const
	{
		return is_open && (bool)(*framesinstream);
	}
};

class Writer: public Stream
{
protected:
	std::shared_ptr<std::ostream> framesoutstream;
public:
	
	Writer(const std::string& filename,
		const Size& tsize,
		const uint32_t tchannels=3,
		const uint32_t ttypewidth=1,
		const double tframerate=29.97,
		const std::string& extra_encode_ffmpeg_params="",
		const std::string& search_path_override="");
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	void write(const void* buf,size_t num_frames)
	{
		framesoutstream->write((const char*)buf,num_frames*frame_buffer_size);
	}
	operator bool() const
	{
		return is_open && (bool)(*framesoutstream);
	}
};

}

#endif
