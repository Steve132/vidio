#ifndef VIDIO_HPP
#define VIDIO_HPP

#include<memory>
#include<iostream>
#include<cstdint>

namespace vidio
{
struct Size
{
public:
	uint32_t width,height;
};

namespace priv
{
	
class StreamImpl;
class Stream
{
protected:
	std::unique_ptr<StreamImpl> impl;
	std::streambuf* sbuf;
public:
	const Size size;
	const uint32_t channels;
	const uint32_t typewidth;
	const bool is_open;

	const size_t frame_size_bytes;
	
	Stream(StreamImpl* iptr);
	virtual ~Stream();
};

class ReaderImpl;
class WriterImpl;
}


class Reader: public priv::Stream
{
protected:
	std::istream framesinstream; 
public:
	const uint32_t num_frames;
	
	Reader(	const std::string& filename,
		const std::string& extra_decode_ffmpeg_params="",
		const std::string& search_path_override="");
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory.
	bool read(void* buf,size_t num_frames=1)
	{
		return (bool)framesinstream.read((char*)buf,num_frames*frame_size_bytes);
	}
};

class Writer: public priv::Stream
{
protected:
	std::ostream framesoutstream;
public:
	
	Writer(const std::string& filename,
		Size toutsize,
		const uint32_t tchannels=3,
		const uint32_t ttypewidth=1,
		const std::string& extra_encode_ffmpeg_params="",
		const std::string& search_path_override="");
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	void write(const void* buf,size_t num_frames)
	{
		framesoutstream.write((const char*)buf,num_frames*frame_size_bytes);
	}
};

}

#endif
