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

/*
 * POSIX-conforming argument struct for worker routine 'complete_encode_worker'.
 */
typedef struct {
	vector<string> *pFilenames;
	bool *pbFilesFinished;
	int iNumFiles;
	int iThreadId;
	int iProcessedFiles;
} ENC_WRK_ARGS;


/*
 * OBSOLETE - argument struct for obsolete routine encode_chunk_to_buffer_worker
 */
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


/////////////////////
// function prototypes
/////////////////////

/* encode_to_file
 *  Main encoding routine which reads input information from gfp and hdr as well as one or two PCM buffers,
 *  encodes it to MP3 and directly stores the MP3 data in the file given by filename.
 *  Calls lame_encode_buffer, lame_encode_flush, and lame_mp3_tags_fid internally for a complete conversion
 *  process.
 */
int encode_to_file(lame_global_flags *gfp, const FMT_DATA *hdr, const short *leftPcm, const short *rightPcm,
	const int iDataSize, const char *filename);

/* OBSOLETE - encode_chunk_to_buffer
 *  Encodes only a chunk of PCM input data to an MP3 output data buffer. This routine is working fine but
 *  not needed anymore as it is not possible to encode multiple chunks of the same file in parallel.
 */
int encode_chunk_to_buffer(int firstSample, int lastSample, lame_global_flags *gfp, const short *leftPcm,
	const short *rightPcm, unsigned char* outBuffer, const int outBufferSize, int &bytesWritten);

/* OBSOLETE
 *  You can convert a file chunk by chunk but only with a single thread, so there's no sense.
 */
int encode_chunks_to_file(lame_global_flags *gfp, const WAV_HDR *hdr, const short *leftPcm, const short *rightPcm,
	int &bytesWritten, const char *filename, unsigned short numChunks = 1);

/* OBSOLETE
 *  Was trying in the beginning to encode a single MP3 file using multiple worker threads but that's seemingly
 *  not the way to go.
 *  You can theoretically use this routine and sucessfully convert using a single thread, but more than one thread
 *  will inevitably lead to crashes.
 */
int encode_chunks_to_file_multithreaded(lame_global_flags *gfp, const WAV_HDR *hdr, const short *leftPcm,
	const short *rightPcm, int &bytesWritten, const char *filename, unsigned short numThreads = 1);

/////////////////////
// threading worker routines conforming to POSIX interface
/////////////////////

/* complete_encode_worker
 *  Main worker thread routine which is supplied with a list of filenames, a status array indicating which files
 *  are already worked upon, and some additional info via a ENC_WRK_ARGS struct.
 *  As long as there are still unprocessed filenames, this routine will fetch the next free filename, mark it as
 *  processed, and execute the complete conversion from reading .wav to writing .mp3.
 *  No mutexes or locks are required as there is only atomic operation access to the status array, while all
 *  other parameters are read-only in all threads or private to each thread.
 */
void *complete_encode_worker(void* arg);

/* OBSOLETE
 *  See encode_chunks_to_file_multithreaded
 */
void *encode_chunk_to_buffer_worker(void* arg);



#endif // __LAME_INTERFACE_H_