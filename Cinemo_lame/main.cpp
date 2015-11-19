#include <cassert>
#include <iostream>
#include <fstream>
#include "lame.h"
#include "pthread.h"
#include "dirent.h"
#include <list>
#include <string>
#include <sstream>
#include <vector>

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
int encode_to_file(lame_global_flags *gfp, const WAV_HDR *hdr, const short *leftPcm, const short *rightPcm,
	const char *filename);
int encode_chunk_to_buffer(int firstSample, int lastSample, lame_global_flags *gfp, const short *leftPcm,
	const short *rightPcm, unsigned char* outBuffer, const int outBufferSize, int &bytesWritten);

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

int encode_to_file(lame_global_flags *gfp, const WAV_HDR *hdr, const short *leftPcm, const short *rightPcm,
	const char *filename)
{
	int numSamples = hdr->dataSize / hdr->bytesPerSample;

	// finish configuration in gfp
	lame_set_num_samples(gfp, numSamples);
	lame_set_num_channels(gfp, hdr->numChannels);

	int mp3BufferSize = numSamples * 5 / 4 + 7200; // worst case estimate
	unsigned char *mp3Buffer = new unsigned char[mp3BufferSize];

	// call to lame_encode_buffer
	int mp3size = lame_encode_buffer(gfp, (short*)leftPcm, (short*)rightPcm, numSamples, mp3Buffer, mp3BufferSize);
	if (!mp3size > 0) {
		delete[] mp3Buffer;
		return EXIT_FAILURE;
	}

	// write to file
	FILE *out = fopen(filename, "wb+");
	fwrite((void*)mp3Buffer, sizeof(unsigned char), mp3size, out);

	// call to lame_encode_flush
	int flushSize = lame_encode_flush(gfp, mp3Buffer, mp3BufferSize);

	// write flushed buffers to file
	fwrite((void*)mp3Buffer, sizeof(unsigned char), flushSize, out);

	// call to lame_mp3_tags_fid (might be omitted)
	lame_mp3_tags_fid(gfp, out);

	fclose(out);
	delete[] mp3Buffer;

	cout << "Wrote " << mp3size + flushSize << " bytes." << endl;
	return EXIT_SUCCESS;
}

typedef struct {
	int firstSample;
	int lastSample;
	lame_global_flags *gfp;
	const short *leftPcm;
	const short *rightPcm;
	unsigned char *outBuffer;
	int outBufferSize;
	int *bytesWritten;
} CHNK_TO_BFR_ARGS;

// Ensure that all settings in GFP have been set correctly before calling this routine!
int encode_chunk_to_buffer(int firstSample, int lastSample, lame_global_flags *gfp, const short *leftPcm,
	const short *rightPcm, unsigned char* outBuffer, const int outBufferSize, int &bytesWritten)
{
	if (lastSample - firstSample < 0) return EXIT_FAILURE;

	int numSamples = lastSample - firstSample;

	bytesWritten = lame_encode_buffer(gfp, &leftPcm[firstSample], &rightPcm[firstSample], numSamples, outBuffer,
		outBufferSize);
	return EXIT_SUCCESS;
}

void *encode_chunk_to_buffer_worker(void* arg)
{
	CHNK_TO_BFR_ARGS *args = (CHNK_TO_BFR_ARGS*)arg;
	int numSamples = args->lastSample - args->firstSample;

	printf("+--------------------------------+\n"
		   "| Thread Samples %d-%d (%d)          \n" 
		   "| GFP addr. 0x%x                   \n"
		   "| PCM_L addr. 0x%x                 \n"
		   "| PCM_R addr. 0x%x                 \n"
		   "| outBuffer addr. 0x%x             \n"
		   "| outBufferSize %d               \n",
		   "+--------------------------------+\n",
		   args->firstSample, args->lastSample, numSamples, args->gfp, args->leftPcm, args->rightPcm,
		   args->outBuffer, args->outBufferSize);

	int ret = lame_encode_buffer(args->gfp, &(args->leftPcm[args->firstSample]), &(args->rightPcm[args->firstSample]),
		numSamples,	args->outBuffer, args->outBufferSize);

	*(args->bytesWritten) = ret;
	return NULL;
}

int encode_chunks_to_file(lame_global_flags *gfp, const WAV_HDR *hdr, const short *leftPcm, const short *rightPcm,
	int &bytesWritten, const char *filename, unsigned short numChunks=1)
{
	int numSamples = hdr->dataSize / hdr->bytesPerSample;
	int samplesPerChunk = numSamples / numChunks + 1;
	int chunkSize = samplesPerChunk * 5 / 4 + 7200; // worst case estimate in bytes

	bytesWritten = 0;

	// allocate output buffers
	unsigned char **outBuffers = new unsigned char*[numSamples];
	int *writtenInChunk = new int[numSamples];
	for (int i = 0; i < numChunks; i++) {
		outBuffers[i] = new unsigned char[chunkSize];
	}

	// encode chunks
	for (int i = 0; i < numChunks; i++) {
		int firstSample = i * samplesPerChunk;
		int lastSample = firstSample + samplesPerChunk - 1;
		if (lastSample >= numSamples) lastSample = numSamples - 1;

		int ret = encode_chunk_to_buffer(firstSample, lastSample, gfp, leftPcm, rightPcm, outBuffers[i],
			chunkSize, writtenInChunk[i]);
		cout << "Chunk " << i << " (samples " << firstSample << "-" << lastSample << "): Wrote "
			<< writtenInChunk[i] << " bytes." << endl;
	}

	// write chunk buffers to file
	FILE *out = fopen(filename, "wb+");
	for (int i = 0; i < numChunks; i++) {
		fwrite((void*)outBuffers[i], sizeof(unsigned char), writtenInChunk[i], out);
		bytesWritten += writtenInChunk[i];
	}


	// clear temporary buffers
	for (int i = 0; i < numChunks; i++) {
		delete[] outBuffers[i];
	}
	delete[] outBuffers;
	delete[] writtenInChunk;

	// flush buffers
	unsigned char *flushBuffer = new unsigned char[32768]; // just provide enough buffer size, lower might be fine
	int flushSize = lame_encode_flush(gfp, flushBuffer, 32768);
	
	fwrite((void*)flushBuffer, sizeof(unsigned char), flushSize, out);
	bytesWritten += flushSize;
	delete[] flushBuffer;

	lame_mp3_tags_fid(gfp, out);

	fclose(out);

	cout << "Wrote " << bytesWritten << " bytes in total." << endl;
	return EXIT_SUCCESS;
}

int encode_chunks_to_file_multithreaded(lame_global_flags *gfp, const WAV_HDR *hdr, const short *leftPcm,
	const short *rightPcm, int &bytesWritten, const char *filename, unsigned short numThreads = 1)
{
	int numChunks = numThreads;
	int numSamples = hdr->dataSize / hdr->bytesPerSample;
	int samplesPerChunk = numSamples / numChunks + 1;
	int chunkSize = samplesPerChunk * 5 / 4 + 7200; // worst case estimate in bytes

	bytesWritten = 0;

	// allocate threads array and argument structs
	pthread_t *threads = new pthread_t[numThreads];
	CHNK_TO_BFR_ARGS *threadArgs = (CHNK_TO_BFR_ARGS*) malloc(numThreads * sizeof(CHNK_TO_BFR_ARGS));

	// allocate output buffers
	unsigned char **outBuffers = new unsigned char*[numSamples];
	int *writtenInChunk = new int[numSamples];
	for (int i = 0; i < numChunks; i++) {
		outBuffers[i] = new unsigned char[chunkSize];
	}

	// assemble thread argument structs
	for (int i = 0; i < numChunks; i++) {
		int firstSample = i * samplesPerChunk;
		int lastSample = firstSample + samplesPerChunk - 1;
		if (lastSample >= numSamples) lastSample = numSamples - 1;

		// assemble argument struct
		threadArgs[i].firstSample = firstSample;
		threadArgs[i].lastSample = lastSample;
		threadArgs[i].gfp = gfp;
		threadArgs[i].leftPcm = leftPcm;
		threadArgs[i].rightPcm = rightPcm;
		threadArgs[i].outBuffer = outBuffers[i];
		threadArgs[i].outBufferSize = chunkSize;
		threadArgs[i].bytesWritten = &writtenInChunk[i];
	}

	// spawn threads
	for (int i = 0; i < numChunks; i++) {
		int ret = pthread_create(&threads[i], NULL, encode_chunk_to_buffer_worker, (void*)&threadArgs[i]);
	}

	// let them do work and synchronize
	for (int i = 0; i < numChunks; i++) {
		int ret = pthread_join(threads[i], NULL);
		if (ret != 0) {
			cerr << "A POSIX thread error occured. Quitting." << endl;
			exit(ret);
		}
	}
	
	// write chunk buffers to file
	FILE *out = fopen(filename, "wb+");
	for (int i = 0; i < numChunks; i++) {
		fwrite((void*)outBuffers[i], sizeof(unsigned char), writtenInChunk[i], out);
		bytesWritten += writtenInChunk[i];
	}


	// clear temporary buffers
	for (int i = 0; i < numChunks; i++) {
		delete[] outBuffers[i];
	}
	delete[] outBuffers;
	delete[] writtenInChunk;

	// flush buffers
	unsigned char *flushBuffer = new unsigned char[32768]; // just provide enough buffer size, lower might be fine
	int flushSize = lame_encode_flush(gfp, flushBuffer, 32768);

	fwrite((void*)flushBuffer, sizeof(unsigned char), flushSize, out);
	bytesWritten += flushSize;
	delete[] flushBuffer;

	lame_mp3_tags_fid(gfp, out);

	fclose(out);
	delete[] threads;
	free(threadArgs);

	cout << "Wrote " << bytesWritten << " bytes in total." << endl;
	return EXIT_SUCCESS;
}

bool string_ends_with(const string &fullString, const string &subString)
{
	if (subString.length() > fullString.length()) return false;
	else {
		// lowercase conversion
		string fullString_l = fullString;
		for (int i = 0; i < fullString_l.length(); i++) {
			if ('A' <= fullString_l[i] && fullString_l[i] <= 'Z')
				fullString_l[i] = fullString_l[i] - ('Z' - 'z');
		}

		string subString_l = subString;
		for (int i = 0; i < subString_l.length(); i++) {
			if ('A' <= subString_l[i] && subString_l[i] <= 'Z')
				subString_l[i] = subString_l[i] - ('Z' - 'z');
		}
		return (fullString_l.compare(fullString_l.length() - subString_l.length(), subString_l.length(),
			subString_l) == 0);
	}
}

list<string> parse_directory(const char *dirname)
{
	DIR *dir;
	dirent *ent;
	list<string> dirEntries;

	if ((dir = opendir(dirname)) != NULL) {
		// list directory
		while ((ent = readdir(dir)) != NULL) {
			dirEntries.push_back(string(ent->d_name));
		}
		closedir(dir);
	} else {
		cerr << "Unable to parse directory." << endl;
	}

	return dirEntries;
}


typedef struct {
	vector<string> *pFilenames;
	bool *pbFilesFinished;
	int iNumFiles;
	int iThreadId;
} ENC_WRK_ARGS;

void *complete_encode_worker(void* arg)
{
	ENC_WRK_ARGS *args = (ENC_WRK_ARGS*)arg; // parse argument struct

	int iNumFilesProcessed = 0;

	while (true) {
		cout << "Checking for work\n";
		// determine which file to process next
		bool bFoundWork = false;
		int iFileIdx = -1;
		for (int i = 0; i < args->iNumFiles; i++) {
			if (!args->pbFilesFinished[i]) {
				// LOCK HERE
				args->pbFilesFinished[i] = true; // mark as being worked on
				// UNLOCK
				iFileIdx = i;
				bFoundWork = true;
				break;
			}
		}

		if (!bFoundWork) {// done yet?
			ostringstream out;
			out << "Thread " << args->iThreadId << " processed " << iNumFilesProcessed << " files." << endl;
			printf(out.str().c_str());
			return NULL; // break
		}
		string sMyFile = args->pFilenames->at(iFileIdx);
		string sMyFileOut = sMyFile.substr(0, sMyFile.length() - 3) + "mp3";

		// start working
		WAV_HDR *hdr = NULL;
		short *leftPcm = NULL, *rightPcm = NULL;
		// init encoding params
		lame_global_flags *gfp = lame_init();
		lame_set_brate(gfp, 192); // increase bitrate
		lame_set_quality(gfp, 3); // increase quality level
		lame_set_bWriteVbrTag(gfp, 0);

		// check params
		int ret = lame_init_params(gfp);
		if (ret != 0) {
			cerr << "Invalid encoding parameters! Quitting now." << endl;
			exit(0);
		}

		// parse wave file
		printf("Parsing %s ...\n", sMyFile.c_str());
		ret = read_wave(sMyFile.c_str(), hdr, leftPcm, rightPcm);
		if (ret != EXIT_SUCCESS) {
			printf("Error in file %s! Skipping.\n", sMyFile.c_str());
			continue; // see if there's more to do
		}

		// encode to mp3
		ret = encode_to_file(gfp, hdr, leftPcm, rightPcm, sMyFileOut.c_str());
		if (ret != EXIT_SUCCESS) {
			cerr << "Unable to encode mp3." << endl;
			continue;
		}

		++iNumFilesProcessed;

		lame_close(gfp);
	}
}

int main(void)
{
	int ret;
	const int NUM_THREADS = 2;
	cout << "LAME version: " << get_lame_version() << endl;

	// parse directory
	list<string> files = parse_directory(".");
	vector<string> wavFiles;
	for (list<string>::iterator it = files.begin(); it != files.end(); it++) {
		// check if it's a wave file
		if (string_ends_with(*it, string(".wav"))) {
			wavFiles.push_back(*it);
		}
	}

	int numFiles = wavFiles.size();

	cout << "Found " << numFiles << " .wav file(s) in directory." << endl;
	if (!(numFiles>0)) return EXIT_SUCCESS;

	// initialize pbFilesFinished array which contains true for all files which are currently already converted
	bool *pbFilesFinished = new bool[numFiles];
	for (int i = 0; i < numFiles; i++) pbFilesFinished[i] = false;


	// initialize threads array and argument arrays
	pthread_t *threads = new pthread_t[NUM_THREADS];
	ENC_WRK_ARGS *threadArgs = (ENC_WRK_ARGS*)malloc(NUM_THREADS * sizeof(ENC_WRK_ARGS));
	for (int i = 0; i < NUM_THREADS; i++) {
		threadArgs[i].iNumFiles = numFiles;
		threadArgs[i].pFilenames = &wavFiles;
		threadArgs[i].pbFilesFinished = pbFilesFinished;
		threadArgs[i].iThreadId = i;
	}

	// create threads
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_create(&threads[i], NULL, complete_encode_worker, (void*)&threadArgs[i]);
	}

	// join threads
	for (int i = 0; i < NUM_THREADS; i++) {
		int ret = pthread_join(threads[i], NULL);
		if (ret != 0) {
			cerr << "A POSIX thread error occured." << endl;
		}
	}


	delete[] threads;
	delete[] threadArgs;

	cout << "Done." << endl;
	return EXIT_SUCCESS;
}