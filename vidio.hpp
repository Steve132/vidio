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

class SampleFormat
{
public:
	std::string codec;
	std::string layout;
	size_t nchannels;
	std::string data_type;
	size_t nbytes;
	SampleFormat(){};
	SampleFormat(const std::string& _codec,
		const std::string& _layout,
		const size_t& _nchannels,
		const std::string& _data_type) :
		codec(_codec),
		layout(_layout),
		nchannels(_nchannels),
		data_type(_data_type),
		nbytes(get_sample_nbytes(_data_type)){};
	~SampleFormat(){};
	std::size_t get_sample_nbytes(const std::string& sample_data_type) const;
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
    struct PixFormat:public PixelFormat
    {
        bool readable,writable;
    };

    FFMPEG_Install(const std::vector<std::string>& extra_ffmpeg_locations={});
    ~FFMPEG_Install();
    
    const std::unordered_map<std::string,PixFormat>& pixformats() const;
	const std::string& ffmpeg_path() const;
    
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
	Reader(const std::vector<std::string>& ffmpeg_input_args,const std::string& pixelformat="",const FFMPEG_Install& install=FFMPEG_Install());
	const PixelFormat& pixelformat() const;
    const PixelFormat& native_pixelformat() const;
	const SampleFormat& sampleformat() const;
	unsigned int samplerate() const;
	double framerate() const;
	bool good() const;
	Size video_frame_dimensions() const;
	size_t video_frame_bufsize() const;
	
	//buf is a buffer with frame_size_bytes*num_frames bytes of memory.
	bool read_video_frame(void *buf) const;
	bool read_audio_samples(void* buf, const size_t& nsamples) const;
	
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
		const std::string& pixelfmt="rgba",
		const std::string& encode_ffmpeg_params="",
		const FFMPEG_Install& install=FFMPEG_Install());

	const PixelFormat& pixelformat() const;
    const PixelFormat& native_pixelformat() const;
	const SampleFormat& sampleformat() const;
	unsigned int samplerate() const;
	double framerate() const;
	bool good() const;
	Size video_frame_dimensions() const;
	size_t video_frame_bufsize() const;

	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	bool write_video_frame(const void* buf) const;
	bool write_audio_samples(const void* buf, const size_t& nsamples) const;

	operator bool() const
	{
		return good();
	}

	~Writer();
};

}


#endif
