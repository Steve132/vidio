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
std::unique_ptr<T[]> read_samples_(vidio::Reader& reader, const size_t& nsamples)
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
		cout << "Read Sample: " << sample << endl;
	}
	return samplesbuf;
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

std::unique_ptr<int16_t[]> reader_test(const size_t& nsamples, const std::string& filename = "output.wav")
{
	vidio::Reader reader(filename);
	unsigned int sample_rate = reader.samplerate();
	vidio::SampleFormat sample_fmt = reader.sampleformat();
	cout << "Reader sample_rate: " << sample_rate << " sample_fmt codec: " << sample_fmt.codec << " layout: " << sample_fmt.layout << " nchannels: " << sample_fmt.nchannels << " data type: " << sample_fmt.data_type << endl;
	cout << "Reading audio samples from " << filename << "." << std::endl;

	float nseconds = .001;//nsamples = sample_rate * nseconds;
	//read_samples(reader, nsamples); // generic for type
	return read_samples_<int16_t>(reader, nsamples); // for our verification test
}

void writer_test(int16_t* samplesbuf, const size_t& nsamples, const std::string& filename = "output.wav")
{
	vidio::Size empty_size;
	empty_size.width=0;
	empty_size.height=0;
	vidio::Writer writer(filename,empty_size,0);
	unsigned int sample_rate = writer.samplerate();
	vidio::SampleFormat sample_fmt = writer.sampleformat();
	cout << "Writer sample_rate: " << sample_rate << " sample_fmt codec: " << sample_fmt.codec << " layout: " << sample_fmt.layout << " nchannels: " << sample_fmt.nchannels << " data type: " << sample_fmt.data_type << endl;

	if(writer.write_audio_samples(samplesbuf, nsamples))
	{
		std::cout << "Wrote audio samples to " << filename << "." << std::endl;
	}
}

int main(int argc,char** argv)
{
	// first 10 samples of sine.wav
	size_t nsamples = 10;
	int16_t samplesbuf[nsamples] = {191,1505,3141,4600,6051,7650,9167,10526,11898,13294};
	writer_test(samplesbuf,nsamples);
	std::unique_ptr<int16_t[]> resultbuf = reader_test(nsamples);
	size_t ncorrect = 0;
	for(size_t sample_ind = 0; sample_ind < nsamples; sample_ind++)
	{
		if(resultbuf[sample_ind] == samplesbuf[sample_ind])
		{
			ncorrect++;
		}
		else
		{
			std::cout << "Non-matching read from written samples.  Written value:" << samplesbuf[sample_ind] << " vs. Read value: " << resultbuf[sample_ind] << std::endl;
		}
	}
	if(ncorrect == nsamples)
	{
		std::cout << "All audio samples written and read correctly." << std::endl;
	}
	return 0;
}
