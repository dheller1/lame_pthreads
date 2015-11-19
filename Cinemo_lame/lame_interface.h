#ifndef __LAME_INTERFACE_H_
#define __LAME_INTERFACE_H_

#include <vector>
#include <sstream>
#include "lame.h"
#include "wave.h"
#include "pthread.h"

using namespace std;

/////////////////////
// interface structs for POSIX worker routine calls
/////////////////////
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

typedef struct {
	vector<string> *pFilenames;
	bool *pbFilesFinished;
	int iNumFiles;
	int iThreadId;
} ENC_WRK_ARGS;

/////////////////////
// function prototypes
/////////////////////
int encode_to_file(lame_global_flags *gfp, const WAV_HDR *hdr, const short *leftPcm, const short *rightPcm,
	const char *filename);

int encode_chunk_to_buffer(int firstSample, int lastSample, lame_global_flags *gfp, const short *leftPcm,
	const short *rightPcm, unsigned char* outBuffer, const int outBufferSize, int &bytesWritten);



int encode_chunks_to_file(lame_global_flags *gfp, const WAV_HDR *hdr, const short *leftPcm, const short *rightPcm,
	int &bytesWritten, const char *filename, unsigned short numChunks = 1);

int encode_chunks_to_file_multithreaded(lame_global_flags *gfp, const WAV_HDR *hdr, const short *leftPcm,
	const short *rightPcm, int &bytesWritten, const char *filename, unsigned short numThreads = 1);

/////////////////////
// threading worker routines conforming to POSIX interface
/////////////////////
void *complete_encode_worker(void* arg);

/* */
void *encode_chunk_to_buffer_worker(void* arg);



#endif // __LAME_INTERFACE_H_