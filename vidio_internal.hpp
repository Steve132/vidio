#ifndef VIDIO_INTERNAL_HPP
#define VIDIO_INTERNAL_HPP

namespace vidio
{
namespace platform
{
//platform-specific code here
std::streambuf* create_process_reader_streambuf(const std::string& cmd);
std::streambuf* create_process_writer_streambuf(const std::string& cmd);
std::string get_default_ffmpeg_prefix();
}
}

#endif
