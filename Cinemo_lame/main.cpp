#include <cassert>
#include <iostream>
#include <fstream>
#include "lame.h"

using namespace std;

typedef struct {
	char rID[4]; // "RIFF"
	int fileLen; // length of file minus 8.
	char wID[8]; // "WAVEfmt "
	int pcmHeaderLength; // 16
	short fmtVersion; 
	short numChannels; // 1: Mono, 2: Stereo
	int sampleRate; // samples per sec
	int byteRate; // Bytes per sec
	short bytesPerSample;
	short bitsPerSample;
	char dID[4]; // "data"
	int dataSize; // size of data array
} WAV_HDR;


// function prototypes
int read_wave(const char *filename, WAV_HDR* &hdr, short* &leftPcm, short* &rightPcm);
int check_wave_header(const WAV_HDR *hdr);
void get_pcm_channels_from_wave(ifstream &file, const WAV_HDR* hdr, short* &leftPcm, short* &rightPcm);

// function implementations
WAV_HDR* read_wave_header(ifstream &file)
{
	WAV_HDR* hdr = new WAV_HDR;

	if (!file.is_open()) return NULL; // check if file is open
	file.seekg(0, ios::beg); // rewind file

	cout << "Attempting to read header (" << sizeof(WAV_HDR) << " bytes)..." << endl;
	file.read((char*)hdr, sizeof(WAV_HDR));

	check_wave_header(hdr);

	return hdr;
}

int check_wave_header(const WAV_HDR *hdr)
{
	// check ID strings
	if (0 != strncmp(hdr->rID, "RIFF", 4)) return EXIT_FAILURE;
	if (0 != strncmp(hdr->wID, "WAVEfmt ", 8)) return EXIT_FAILURE;
	if (0 != strncmp(hdr->dID, "data", 4)) return EXIT_FAILURE;

	cout << "Length of file-8:  " << hdr->fileLen << endl; // file length
	cout << "PCM header length: " << hdr->pcmHeaderLength << endl;
	cout << "Format version:    " << hdr->fmtVersion << endl;
	cout << "Channels:          " << hdr->numChannels << endl;
	cout << "Sample rate:       " << hdr->sampleRate << " Hz" << endl;
	cout << "Byte rate:         " << hdr->byteRate << " B/s" << endl;
	cout << "Bytes per sample:  " << hdr->bytesPerSample << endl;
	cout << "Bits per sample:   " << hdr->bitsPerSample << endl;
	cout << "Length of data:    " << hdr->dataSize << endl;

	cout << "WAV file seems okay." << endl;
	cout << "Number of samples (per channel): " << hdr->dataSize / hdr->bytesPerSample << endl;
	return EXIT_SUCCESS;
}

void get_pcm_channels_from_wave(ifstream &file, const WAV_HDR* hdr, short* &leftPcm, short* &rightPcm)
{
	unsigned int idx;
	
	int curChannel = 0; // index into pcmChannels array
	int numSamples = hdr->dataSize / hdr->bytesPerSample;

	// allocate PCM arrays
	leftPcm = new short[hdr->dataSize / hdr->numChannels / sizeof(short)];
	if (hdr->numChannels > 1)
		rightPcm = new short[hdr->dataSize / hdr->numChannels / sizeof(short)];

	// capture each frame
	file.seekg(sizeof(WAV_HDR)); // set file pointer to beginning of data array after the header
	for (idx = 0; idx < numSamples; idx++)
	{
		file.read((char*)&leftPcm[idx], hdr->bytesPerSample / hdr->numChannels);
		if (hdr->numChannels>1)
			file.read((char*)&rightPcm[idx], hdr->bytesPerSample / hdr->numChannels);
	}

	cout << "File parsed successfully." << endl;
}

int read_wave(const char *filename, WAV_HDR* &hdr, short* &leftPcm, short* &rightPcm)
{
	size_t size;
	
	ifstream inFile(filename, ios::in | ios::binary);
	if (inFile.is_open()) {
		// determine size and allocate buffer
		inFile.seekg(0, ios::end);
		size = inFile.tellg();
		
		cout << "Opened file. Allocating " << size << " bytes." << endl;

		// parse file
		hdr = read_wave_header(inFile);
		get_pcm_channels_from_wave(inFile, hdr, leftPcm, rightPcm);
		inFile.close();

		// cleanup and return
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

int main(void)
{
	WAV_HDR *hdr=NULL;
	short *leftPcm=NULL, *rightPcm=NULL;
	int ret;
	cout << "LAME version: " << get_lame_version() << endl;

	// init encoding params
	lame_global_flags *gfp = lame_init();
	lame_set_brate(gfp, 192); // increase bitrate
	lame_set_quality(gfp, 3); // increase quality level
	lame_set_bWriteVbrTag(gfp, 0);
	
	// check params
	ret = lame_init_params(gfp);
	if (ret != 0) {
		cerr << "Invalid encoding parameters! Quitting now." << endl;
		return EXIT_FAILURE;
	} else cout << "Parameters okay." << endl;


	// parse wave file
	ret = read_wave("wail-long.wav", hdr, leftPcm, rightPcm);
	if (ret != EXIT_SUCCESS) {
		cerr << "Couldn't read wave input file." << endl;
		return EXIT_FAILURE;
	}

	// encode to mp3
	int numSamples = hdr->dataSize / hdr->bytesPerSample;

	lame_set_num_samples(gfp, numSamples);
	lame_set_num_channels(gfp, hdr->numChannels);

	int mp3BufferSize = numSamples * 5 / 4 + 7200; // worst case estimate
	unsigned char *mp3Buffer = new unsigned char[mp3BufferSize];

	int mp3size = lame_encode_buffer(gfp, (short*)leftPcm, (short*)rightPcm, numSamples,
		mp3Buffer, mp3BufferSize);
	cout << "Return code of lame_encode_buffer: " << mp3size << endl;
	
	// write output
	FILE* out = fopen("test.mp3", "wb+");
	fwrite((void*)mp3Buffer, 1, mp3size, out);

	// flush buffers
	ret = lame_encode_flush(gfp, mp3Buffer, mp3BufferSize);
	cout << "Return code of lame_encode_flush:  " << ret << endl;

	fwrite((void*)mp3Buffer, sizeof(unsigned char), ret, out);
	lame_mp3_tags_fid(gfp, out);
	
	fclose(out);

	delete[] mp3Buffer;
	
	lame_close(gfp); // cleanup
	return EXIT_SUCCESS;
}