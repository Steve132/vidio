#include<iostream>
#include<string>
#include<exception>
#include<vector>
#include "vidio.hpp"
using namespace std;

/*
Need to STORE standard ffmpeg -sample_fmts in code:
name   depth
u8        8 
s16      16 
s32      32 
flt      32 
dbl      64 
u8p       8 
s16p     16 
s32p     32 
fltp     32 
dblp     64 
s64      64 
s64p     64 
*/

int main(int argc,char** argv)
{
	vidio::Reader reader("sine.wav");
	unsigned int sample_rate = reader.samplerate();
	vidio::SampleFormat sample_fmt = reader.sampleformat();
	cout << "sample_rate: " << sample_rate << " sample_fmt codec: " << sample_fmt.codec << " layout: " << sample_fmt.layout << " nchannels: " << sample_fmt.nchannels << " data type: " << sample_fmt.data_type << endl;

	float nseconds = .001;
	size_t nsamples = 100;//sample_rate * nseconds;

	// create buffer for samples with parsed data type
	// TODO: template for read_audio_samples on data type and to map ffmpeg -sample_fmts to unique_ptr samplesbuf allocation.
	if(sample_fmt.data_type != "s16")
	{
		throw std::runtime_error("Vidio: only valid input audio sample data type is s16.");
	}
	size_t bps = 2; // bytes per sample from data type of s16
	size_t sample_bytes_sz = bps * sample_fmt.nchannels;
	size_t audio_samplebuf_bytes_sz = sample_bytes_sz * nsamples;


	std::unique_ptr<int16_t[]> samplesbuf(new int16_t[sample_fmt.nchannels * nsamples]);
	reader.read_audio_samples(samplesbuf.get(), nsamples);

	for(size_t sample_ind = 0; sample_ind < nsamples; sample_ind++)
	{
		int16_t sample = (int16_t)samplesbuf[sample_ind];
		cout << "SAMPLE: " << sample << endl;
	}


	return 0;
}
