#ifndef VIDIO_INTERNAL_HPP
#define VIDIO_INTERNAL_HPP
#include<string>
#include<vector>
#include<iostream>
#include<memory>

namespace vidio
{
namespace platform
{
//platform-specific code here
std::shared_ptr<std::istream> create_process_reader_stream(const std::string& cmd);
std::shared_ptr<std::ostream> create_process_writer_stream(const std::string& cmd);
std::streambuf* create_process_reader_streambuf(const std::string& cmd);
std::streambuf* create_process_writer_streambuf(const std::string& cmd);
const std::vector<std::string>& get_default_ffmpeg_search_locations();
}
}

#endif
