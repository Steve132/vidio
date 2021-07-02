#ifndef VIDIO_HPP
#define VIDIO_HPP

#include<memory>
#include<iostream>
#include<cstdint>
#include<string>
#include<vector>

namespace vidio
{

struct Size
{
	size_t width;
	size_t height;
};

struct PixelFormat
{
	std::string name;
	unsigned num_components;
	unsigned bits_per_pixel;
};

class Reader
{
protected:
	class Impl;
	std::shared_ptr<Impl> impl;
public:
	Reader(const std::string& filename,const std::string& pixelformat="",const std::vector<std::string>& extra_ffmpeg_locations={});

	const PixelFormat& pixelformat() const;
	double framerate() const;
	bool good() const;
	Size size() const;
	size_t framesize() const {
		Size sz=size();
		return sz.width*sz.height*(pixelformat().bits_per_pixel/8);
	}

	bool read_video_frame(void *buf) const;
	bool read_audio_frame(void* buf) const;
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory.
	
	operator bool() const
	{
		return good();
	}
};

class Writer
{
protected:
	class Impl;
	std::shared_ptr<Impl> impl;
public:
	
	Writer(const std::string& filename,
		const Size& size,
		double framerate,
		const std::string& fmt="rgba",
		const std::string& encode_ffmpeg_params="",
		const std::vector<std::string>& extra_ffmpeg_locations={});
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	bool write_video_frame(const void* buf) const;
	bool write_audio_frame(const void* buf) const;

	operator bool() const
	{
		return is_open && (bool)(*framesoutstream);
	}
};


#endif
