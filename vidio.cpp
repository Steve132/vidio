#include "vidio.hpp"
#include "vidio_internal.hpp"
#include <list>
#include <algorithm>
#include <iterator>

std::string get_ffmpeg_prefix(const std::string& usr_override="")
{
	std::list<std::string> possible_locations;
	if(usr_override != "")
	{
		possible_locations.push_back(usr_override);
	}
	
	const std::vector<std::string&> pform=platform::get_default_ffmpeg_prefix();
	copy(pform.cbegin(),pform.cend(),std::back_inserter(possible_locations));
	
	for(auto pl:possible_locations)
	{
		try
		{
			platform::create_process_reader_streambuf(pl);
			return pl;
		}
		catch(const std::exception& e)
		{}
	}
	throw std::runtime_error("No executable ffmpeg process found");
}