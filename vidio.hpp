#ifndef VIDIO_HPP
#define VIDIO_HPP

#include<memory>
#include<iostream>
#include<cstdint>
#include<string>
#include<vector>
#include<unordered_map>

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

class Reader;
class Writer;
struct FFMPEG_Install
{
protected:
    class Impl;
    std::shared_ptr<Impl> impl;
    friend class Reader;
    friend class Writer;
public:
    FFMPEG_Install(const std::vector<std::string>& extra_ffmpeg_locations={});
    ~FFMPEG_Install();
    
    const std::unordered_map<std::string,PixelFormat>& valid_read_pixelformats() const;
    const std::unordered_map<std::string,PixelFormat>& valid_write_pixelformats() const;
	const std::string ffmpeg_path() const;
    
    bool good() const;
    operator bool(){ return good(); }
};

class Reader
{
protected:
	class Impl;
	std::shared_ptr<Impl> impl;
public:
	Reader(const std::string& filename,const std::string& pixelformat="",const FFMPEG_Install& install=FFMPEG_Install());

	const PixelFormat& pixelformat() const;
	double framerate() const;
	bool good() const;
	Size video_frame_dimensions() const;
	size_t video_frame_bufsize() const {
		Size sz=video_frame_dimensions();
		return sz.width*sz.height*(pixelformat().bits_per_pixel/8);
	}
	
	bool read_video_frame(void *buf) const;
	bool read_audio_frame(void* buf) const;
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory.
	
	operator bool() const
	{
		return good();
	}
	
	~Reader();
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
		const FFMPEG_Install& install=FFMPEG_Install());

	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	bool write_video_frame(const void* buf) const;
	bool write_audio_frame(const void* buf) const;
    bool good() const;

	operator bool() const
	{
		return good();
	}
};

}


#endif
