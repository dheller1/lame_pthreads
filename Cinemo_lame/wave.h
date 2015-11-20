#ifndef __WAVE_H_
#define __WAVE_H_

#include <fstream>
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>

using namespace std;

/* Initial header of WAV file */
typedef struct {
	char rID[4]; // "RIFF"
	unsigned int fileLen; // length of file minus 8 for "RIFF" and fileLen.
	char wID[4]; // "WAVE"
} RIFF_HDR;

/* Chunk header and data for 'fmt ' format chunk in WAV file (required).
 * This information determines how the PCM input streams must be interpreted.
 */
typedef struct {
	char ID[4]; // "fmt "
	unsigned int chunkSize; // should be 16
	unsigned short wFmtTag; // 0x01 for PCM - other modes unsupported
	unsigned short wChannels; // number of channels (1 mono, 2 stereo)
	unsigned int dwSamplesPerSec; // e.g. 44100
	unsigned int dwBytesPerSec;   // e.g. 4*44100
	unsigned short wBlockAlign; // bytes per sample (all channels, e.g. 4)
	unsigned short wBitsPerSample; // bits per sample and channel, e.g. 16
} FMT_DATA;

/* Chunk header for any chunk type in IFF format
 * such as 'fmt ' or 'data' (we ignore everything else).
 */
typedef struct {
	char ID[4]; // any identifier
	unsigned int chunkSize;
} ANY_CHUNK_HDR;


/* OBSOLETE - this was based on incorrect information on the WAV header format. */
/* Struct for wave header (44 bytes) */
typedef struct {
	char rID[4]; // "RIFF"
	int fileLen; // length of file minus 8.
	char wID[8]; // "WAVEfmt "
	int pcmHeaderLength; // 16
	short fmtTag;
	short numChannels; // 1: Mono, 2: Stereo
	int sampleRate; // samples per sec
	int byteRate; // Bytes per sec
	short bytesPerSample;
	short bitsPerSample; // actually per sample and channel (i.e. could be 16 bits / 4 bytes per sample for stereo)
	char dID[4]; // "data"
	int dataSize; // size of data array
} WAV_HDR;


/* read_wave
 *  Read a WAV file given by filename, parsing the 44 byte header into hdr (will be allocated),
 *  allocating arrays for left and right (only left for mono input) PCM channels and returning
 *  pointers to them.
 *  This is the 'does-it-all' routine which you should call from outside, it will subsequently
 *  call read_wave_header, check_wave_header, and get_pcm_channels_from_wave itself.
 */
int read_wave(
	const char*			filename,			/* file to open */
	FMT_DATA*			&hdr,				/* stores header info here (will be allocated) */
	short*				&leftPcm,			/* stores left (or mono) PCM channel here */
	short*				&rightPcm,			/* stores right PCM channel (stereo only) here */
	int					&iDataSize			/* size of data array */
);

/* read_wave_header
 *  Parses the given file stream for the WAV header and 'fmt ' as well as 'data' information.
 *
 *  Return value:
 *    EXIT_SUCCESS or EXIT_FAILURE
 */
int read_wave_header(
	ifstream			&file,				/* file stream to parse from (must be opened for reading) */
	FMT_DATA*			&hdr,				/* stores header info here (will be allocated) */
	int					&iDataSize,			/* stores size of data array here */
	int					&iDataOffset		/* stores data offset (first data byte in input file) here */
);

/* check_riff_header
 * Checks validity of initial WAV header ('RIFF', fileLen, 'WAVE')
 */
int check_riff_header(const RIFF_HDR *rHdr);

/* check_format_data
 * Checks validity of WAV settings provided by hdr.
 */
int check_format_data(const FMT_DATA *hdr);

/* get_pcm_channels_from_wave
*  Allocates buffers for left and (if stereo) right PCM channels and parses data from filestream.
*  Header hdr must have been read before.
*
*  Return value:
*    EXIT_SUCCESS  if header is valid
*    EXIT_FAILURE  if we probably can't handle that file
*/
void get_pcm_channels_from_wave(
	ifstream			&file,				/* file stream to parse from (must be opened for reading) */
	const FMT_DATA*		hdr,				/* pointer to format struct that's already been read */
	short*				&leftPcm,			/* stores left PCM channel here (will be allocated) */
	short*				&rightPcm,			/* stores right PCM channel here (will be allocated for stereo files)*/
	const int			iDataSize,			/* size of PCM data array */
	const int			iDataOffset			/* first PCM array byte in file*/
);



/* OBSOLETE - this was based on incorrect information on the WAV header format.
*  check_wave_header
*  Simple validation of WAV header info provided by hdr.
*  Will write diagnostic output if verbosity level is high enough.
*
*  Return value:
*    EXIT_SUCCESS  if header is valid
*    EXIT_FAILURE  if we probably can't handle that file
*/
int check_wave_header(
	const WAV_HDR*		hdr					/* pointer to header struct which will be validated */
);


#endif //__WAVE_H_
