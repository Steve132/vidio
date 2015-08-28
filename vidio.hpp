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
template<typename Child>
class Stream
{
protected:
	std::unique_ptr<typename Child::Impl> impl;
	std::streambuf* sbuf;
public:
	const Size size;
	const uint32_t channels;
	const uint32_t typewidth;
	const bool is_open;

	const size_t frame_size_bytes;
	
	Stream(typename Child::Impl* iptr);
	
	virtual ~Stream()
	{}
};

}

class Reader: public Stream<Reader>
{
protected:
	class Impl;
	std::istream framesinstream; 
public:
	const uint32_t num_frames;
	
	Reader(	const std::string& filename,
		const std::string& extra_decode_ffmpeg_params=""
		const std::string& search_path_override="");
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	bool read(void* buf,size_t num_frames=1);
};

class Writer: public Stream<Writer>
{
protected:
	class Impl;
	std::ostream framesoutstream;
public:
	
	Writer(const std::string& filename,
		Size toutsize,
		const uint32_t tchannels=3,
		const uint32_t ttypewidth=1,
		const std::string& extra_encode_ffmpeg_params=""
		const std::string& search_path_override="");
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	void write(const void* buf,size_t num_frames=1);
};

}

#endif
