#ifndef __WAVE_H_
#define __WAVE_H_

#include <fstream>
#include <iostream>
#include <cassert>

using namespace std;

/* Struct for wave header (44 bytes) */
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

#endif //__WAVE_H_