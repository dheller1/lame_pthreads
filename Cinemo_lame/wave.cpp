#include "wave.h"

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
	assert(sizeof(WAV_HDR) == 44);
	int idx;

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
	streamoff size;

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