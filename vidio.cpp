#include "vidio.hpp"
#include<unordered_map>

class FFMPEG_Install
{
public:
	FFMPEG_Install(const std::vector<std::string>& additional_search_locations={});
	bool good() const;

	std::string path;
	std::unordered_map<std::string,vidio::PixelFormat> readable_formats; //"O"
	std::unordered_map<std::string,vidio::PixelFormat> writeable_formats; //"I"

	const FFMPEG_Install& instance(const std::vector<std::string>& additional_search_locations={})
	{
		static std::shared_ptr<FFMPEG_Install> def;
		if(!def) { def=std::make_shared<FFMPEG_Install>(additional_search_locations); }
		if(!def) {
			throw std::runtime_error("Error, can't find ffmpeg.");
		}
		return *def;
	}
};


//eventually use HW formats. https://www.ffmpeg.org/doxygen/2.5/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5

namespace vidio
{

class Reader::Impl
{
public:

};

class Writer::Impl
{
public:

};

}
