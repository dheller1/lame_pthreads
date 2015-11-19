#include <cassert>
#include <iostream>
#include <fstream>
#include "lame.h"
#include "pthread.h"

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

	exit(0);
	
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
	/*ret = encode_to_file(gfp, hdr, leftPcm, rightPcm, "test.mp3");
	if (ret != EXIT_SUCCESS) {
		cerr << "Unable to encode to mp3." << endl;
		return EXIT_FAILURE;
	}*/

	int written = 0;
	lame_set_num_channels(gfp, hdr->numChannels);
	lame_set_num_samples(gfp, hdr->dataSize / hdr->bytesPerSample);
	ret = encode_chunks_to_file_multithreaded(gfp, hdr, leftPcm, rightPcm, written, "test.mp3", 2);


	//int numSamples = hdr->dataSize / hdr->bytesPerSample;
	//lame_set_num_channels(gfp, hdr->numChannels);
	//lame_set_num_samples(gfp, numSamples);

	//int bufferSize = numSamples * 5 / 4 + 7200; // worst case estimate
	//unsigned char *outBuffer = new unsigned char[bufferSize];

	//int bytesWritten = 0;

	//// encode in chunks
	//int totalBytes = 0;
	//ret = encode_chunk_to_buffer(0, numSamples/2, gfp, leftPcm, rightPcm, outBuffer, bufferSize, bytesWritten);
	//cout << bytesWritten << endl;
	//totalBytes += bytesWritten;

	//ret = encode_chunk_to_buffer(numSamples / 2 + 1, numSamples - 1, gfp, leftPcm, rightPcm,
	//	&outBuffer[totalBytes], bufferSize, bytesWritten);
	//cout << bytesWritten << endl;
	//totalBytes += bytesWritten;

	//cout << "Total bytes: " << totalBytes << endl;

	//// save to file
	//FILE *out = fopen("test.mp3", "wb+");
	//fwrite((void*)outBuffer, sizeof(unsigned char), totalBytes, out);
	//
	//// flush buffers
	//int bytesFlushed = lame_encode_flush(gfp, outBuffer, bufferSize);
	//cout << "Bytes flushed: " << bytesFlushed << endl;
	//fwrite((void*)outBuffer, sizeof(unsigned char), bytesFlushed, out);

	//lame_mp3_tags_fid(gfp, out);
	//fclose(out);

	//delete[] outBuffer;

	cout << "Done." << endl;
	
	lame_close(gfp); // cleanup
	return EXIT_SUCCESS;
}