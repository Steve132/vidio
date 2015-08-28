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

namespace
{
class Stream
{
public:
	const Size size;
	const uint32_t channels;
	const uint32_t typewidth;
	const bool is_open;
	
	const size_t frame_size_bytes;
	
	Stream(const Size tsize,const uint32_t tchannels,const uint32_t ttypewidth,const bool tis_open):
		size(tsize),channels(tchannels),typewidth(ttypewidth),is_open(tisopen),
		frame_size_bytes(tsize.width*tsize.height*tchannels*ttypewidth)
	{}
	
	virtual ~Stream()
	{}
};

}

class Reader: public Stream
{
private:
	class Impl;
	std::unique_ptr<Impl> impl;
public:
	const uint32_t num_frames;
	
	Reader(const std::string& filename,const std::string& extra_decode_ffmpeg_params="");
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	bool read(void* buf,size_t num_frames=1);
};

class Writer: public Stream
{
private:
	class Impl;
	std::unique_ptr<Impl> impl;
public:
	
	Writer(const std::string& filename,const std::string& extra_encode_ffmpeg_params="");
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	void write(const void* buf,size_t num_frames=1);
};

}

#endif
