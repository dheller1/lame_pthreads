#include "wave.h"

// function implementations
int read_wave_header(ifstream &file, FMT_DATA *&hdr, int &iDataSize, int &iDataOffset)
{
	if (!file.is_open()) return NULL; // check if file is open
	file.seekg(0, ios::beg); // rewind file

	ANY_CHUNK_HDR chunkHdr;

	// read and validate RIFF header first
	RIFF_HDR rHdr;
	file.read((char*)&rHdr, sizeof(RIFF_HDR));
	if (EXIT_SUCCESS != check_riff_header(&rHdr))
		return EXIT_FAILURE;

	// then continue parsing file until we find the 'fmt ' chunk
	bool bFoundFmt = false;
	while (!bFoundFmt && !file.eof()) {
		file.read((char*)&chunkHdr, sizeof(ANY_CHUNK_HDR));
		if (0 == strncmp(chunkHdr.ID, "fmt ", 4)) {
			// rewind and parse the complete chunk
			file.seekg((int)file.tellg()-sizeof(ANY_CHUNK_HDR));
			
			hdr = new FMT_DATA;
			file.read((char*)hdr, sizeof(FMT_DATA));
			bFoundFmt = true;
			break;
		} else {
			// skip this chunk (i.e. the next chunkSize bytes)
			file.seekg(chunkHdr.chunkSize, ios::cur);
		}
	}
	if (!bFoundFmt) { // found 'fmt ' at all?
		cerr << "FATAL: Found no 'fmt ' chunk in file." << endl;
	} else if (EXIT_SUCCESS != check_format_data(hdr)) { // if so, check settings
		delete hdr;
		return EXIT_FAILURE;
	}

	// finally, look for 'data' chunk
	bool bFoundData = false;
	int iSkippedChunks = 0;
	while (!bFoundData && !file.eof()) {
		//printf("Reading chunk at 0x%X:", (int)file.tellg());
		file.read((char*)&chunkHdr, sizeof(ANY_CHUNK_HDR));
		//printf("%s\n", string(chunkHdr.ID, 4).c_str());
		if (0 == strncmp(chunkHdr.ID, "data", 4)) {
			bFoundData = true;
			iDataSize = chunkHdr.chunkSize;
			iDataOffset = file.tellg();
			break;
		} else { // skip chunk
			file.seekg(chunkHdr.chunkSize, ios::cur);
		}
	}
	if (!bFoundData) { // found 'data' at all?
		cerr << "FATAL: Found no 'data' chunk in file." << endl;
		delete hdr;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int check_format_data(const FMT_DATA *hdr)
{
	if (hdr->wFmtTag != 0x01) {
		cerr << "Bad non-PCM format." << endl;
		return EXIT_FAILURE;
	}
	if (hdr->wChannels != 1 && hdr->wChannels != 2) {
		cerr << "Bad number of channels (only mono or stereo supported)." << endl;
		return EXIT_FAILURE;
	}
	if (hdr->chunkSize != 16) {
		cerr << "WARNING: 'fmt ' chunk size seems to be off." << endl;
	}
	if (hdr->wBlockAlign != hdr->wBitsPerSample * hdr->wChannels / 8) {
		cerr << "WARNING: 'fmt ' has strange bytes/bits/channels configuration." << endl;
	}
	
	return EXIT_SUCCESS;
}

int check_riff_header(const RIFF_HDR *rHdr)
{
	if (0 == strncmp(rHdr->rID, "RIFF", 4) && 0 == strncmp(rHdr->wID, "WAVE", 4) && rHdr->fileLen > 0)
		return EXIT_SUCCESS;

	cerr << "Bad RIFF header!" << endl;
	return EXIT_FAILURE;
}

int OBSOLETE_check_wave_header(const WAV_HDR *hdr)
{
	// check ID strings
	if (0 != strncmp(hdr->rID, "RIFF", 4) || 0 != strncmp(hdr->wID, "WAVEfmt ", 8) || 0 != strncmp(hdr->dID, "data", 4)) {
		string rID = string(hdr->rID, 4);
		string wID = string(hdr->wID, 8);
		string dID = string(hdr->dID, 4);
		cerr << "Bad header: rID/wID/dID = '" << rID.c_str() << "', '" << wID.c_str() << "', '" << dID.c_str() << "'" << endl;
		return EXIT_FAILURE;
	}
	else if (hdr->fmtTag != 0x01) {// no PCM?
		cerr << "Bad header: No non-PCM format allowed!" << endl;
		return EXIT_FAILURE;
	}

#ifdef __VERBOSE_
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
#endif
	return EXIT_SUCCESS;
}

void get_pcm_channels_from_wave(ifstream &file, const FMT_DATA* hdr, short* &leftPcm, short* &rightPcm, const int iDataSize,
	const int iDataOffset)
{
	int idx;
	int numSamples = iDataSize / hdr->wBlockAlign;

	// allocate PCM arrays
	leftPcm = new short[iDataSize / hdr->wChannels / sizeof(short)];
	if (hdr->wChannels > 1)
		rightPcm = new short[iDataSize / hdr->wChannels / sizeof(short)];

	// capture each sample
	file.seekg(iDataOffset);// set file pointer to beginning of data array
	for (idx = 0; idx < numSamples; idx++)
	{
		file.read((char*)&leftPcm[idx], hdr->wBlockAlign / hdr->wChannels);
		if (hdr->wChannels>1)
			file.read((char*)&rightPcm[idx], hdr->wBlockAlign / hdr->wChannels);
	}

#ifdef __VERBOSE_
	cout << "File parsed successfully." << endl;
#endif
}

int read_wave(const char *filename, FMT_DATA* &hdr, short* &leftPcm, short* &rightPcm, int &iDataSize)
{
#ifdef __VERBOSE_
	streamoff size;
#endif

	ifstream inFile(filename, ios::in | ios::binary);
	if (inFile.is_open()) {
		// determine size and allocate buffer
		inFile.seekg(0, ios::end);
#ifdef __VERBOSE_
		size = inFile.tellg();
		cout << "Opened file. Allocating " << size << " bytes." << endl;
#endif

		// parse file
		int iDataOffset = 0;
		if (EXIT_SUCCESS != read_wave_header(inFile, hdr, iDataSize, iDataOffset)) {
			return EXIT_FAILURE;
		}
		get_pcm_channels_from_wave(inFile, hdr, leftPcm, rightPcm, iDataSize, iDataOffset);
		inFile.close();

		// cleanup and return
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}
