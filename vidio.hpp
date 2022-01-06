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
	std::size_t get_sample_nbytes(const std::string& sample_data_type) const
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
		const std::string& pixelfmt="rgb24",
		const std::string& encode_ffmpeg_params="",
		const FFMPEG_Install& install=FFMPEG_Install());

	const SampleFormat& sampleformat() const;
	unsigned int samplerate() const;

	//buf is a buffer with frame_size_bytes*num_frames bytes of memory
	bool write_video_frame(const void* buf) const;
	bool write_audio_samples(const void* buf, const size_t& nsamples) const;
    bool good() const;

	operator bool() const
	{
		return good();
	}

	~Writer();
};

}


#endif
