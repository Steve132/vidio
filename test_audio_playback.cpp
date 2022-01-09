#include<iostream>
#include<string>
#include<exception>
#include<vector>
#include "vidio.hpp"
#include "subprocess.hpp"

// mkfifo stuff:
#include <unistd.h>
#include <sys/stat.h>
#include <stdexcept>
#include <cerrno> // read error set by mkfifo with errno
#include <cstring> // strerror for errno for mkfifo

using namespace std;

template<class T>
void read_samples_(vidio::Reader& reader, const size_t& nsamples)
{
	vidio::SampleFormat sample_fmt = reader.sampleformat();
	size_t bps = sizeof(T); // bytes per sample from data type of s16
	size_t sample_bytes_sz = bps * sample_fmt.nchannels;
	size_t audio_samplebuf_bytes_sz = sample_bytes_sz * nsamples;
	std::unique_ptr<T[]> samplesbuf(new T[sample_fmt.nchannels * nsamples]);
	reader.read_audio_samples(samplesbuf.get(), nsamples);

	for(size_t sample_ind = 0; sample_ind < nsamples; sample_ind++)
	{
		T sample = (T)samplesbuf[sample_ind];
		cout << "SAMPLE: " << sample << endl;
	}
}

void read_samples(vidio::Reader& reader, const size_t& nsamples)
{
	vidio::SampleFormat sample_fmt = reader.sampleformat();
	//From ffmpeg -sample_fmts:
	if(sample_fmt.data_type == "u8" || sample_fmt.data_type == "u8p")
		read_samples_<uint8_t>(reader, nsamples);
	else if(sample_fmt.data_type == "s16" || sample_fmt.data_type == "s16p")
		read_samples_<int16_t>(reader, nsamples);
	else if(sample_fmt.data_type == "s32" || sample_fmt.data_type == "s32p")
		read_samples_<int32_t>(reader, nsamples);
	else if(sample_fmt.data_type == "flt" || sample_fmt.data_type == "fltp")
		read_samples_<float>(reader, nsamples);
	else if(sample_fmt.data_type == "dbl" || sample_fmt.data_type == "dblp")
		read_samples_<double>(reader, nsamples);
	else if(sample_fmt.data_type == "s64" || sample_fmt.data_type == "s64p")
		read_samples_<int64_t>(reader, nsamples);
	else
		throw std::runtime_error("Vidio: invalid sample fmt.  Must be valid from ffmpeg -sample_fmts.");
}

void reader_test()
{
	vidio::Reader reader("sine.wav");
	unsigned int sample_rate = reader.samplerate();
	vidio::SampleFormat sample_fmt = reader.sampleformat();
	cout << "sample_rate: " << sample_rate << " sample_fmt codec: " << sample_fmt.codec << " layout: " << sample_fmt.layout << " nchannels: " << sample_fmt.nchannels << " data type: " << sample_fmt.data_type << endl;

	float nseconds = .001;
	size_t nsamples = 10;//sample_rate * nseconds;
	read_samples(reader, nsamples);
}

void writer_test()
{
	// first 10 samples of sine.wav
	size_t nsamples = 10;
	int16_t samplesbuf[nsamples] = {191,1505,3141,4600,6051,7650,9167,10526,11898,13294};

	vidio::Size empty_size;
	empty_size.width=0;
	empty_size.height=0;
	vidio::Writer writer("sine.raw",empty_size,0);
	unsigned int sample_rate = writer.samplerate();
	vidio::SampleFormat sample_fmt = writer.sampleformat();
	cout << "Writer sample_rate: " << sample_rate << " sample_fmt codec: " << sample_fmt.codec << " layout: " << sample_fmt.layout << " nchannels: " << sample_fmt.nchannels << " data type: " << sample_fmt.data_type << endl;

	if(writer.write_audio_samples(samplesbuf, nsamples))
	{
		std::cout << "wrote audio samples." << std::endl;
	}
}

// Isolate test away from Vidio::Writer class to simply Subprocess:
bool write_audio_samples(std::unique_ptr<Subprocess>& ffmpeg_process_audio, const void* buf, const size_t& nsamples)
{
	size_t nchannels = 1;//sampleformat().nchannels;
	size_t nbytes = 2;
	size_t audio_samplebuf_size = nbytes * nsamples * nchannels;
	const uint8_t* tmpbuf = static_cast<const uint8_t*>(buf);
	std::size_t res;
	for(std::size_t bytes_read = 0; bytes_read < audio_samplebuf_size; bytes_read += res)
	{
		std::cout << "Writing bytes." << std::endl;
		res = ffmpeg_process_audio->write_to_stdin(reinterpret_cast<const char*>(tmpbuf + bytes_read), audio_samplebuf_size - bytes_read);
		if(res == 0)
			return false;
	}
	return true;
}

bool write_test(const std::string& filename = "output.wav")
{
	// first 10 samples of sine.wav
	size_t nsamples = 10;
	int16_t samplesbuf[nsamples] = {191,1505,3141,4600,6051,7650,9167,10526,11898,13294};
	const char *const commandLine[] = {"/usr/bin/ffmpeg", "-hide_banner", "-y", "-i", "pipe:0", "-f", "s16le", "-c:a", "pcm_s16le", filename.c_str(), 0};
	std::unique_ptr<Subprocess> ffmpeg_process_audio=std::make_unique<Subprocess>(commandLine, Subprocess::JOIN);
	if(ffmpeg_process_audio == nullptr)
	{
		std::cout << "Could not create ffmpeg audio process!" << std::endl;
		return false;
	}
	std::cout << "calling write audio samples" << std::endl;
	bool retval = write_audio_samples(ffmpeg_process_audio, samplesbuf, nsamples);
	return retval;
}

int main(int argc,char** argv)
{
	//reader_test();
	//writer_test();
	write_test();
	return 0;
}
