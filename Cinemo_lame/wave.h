<<<<<<< 817a0ebca52d56f5fb264ff7a7bc5ef619b9bd94
#ifndef __WAVE_H_
#define __WAVE_H_

#include <fstream>
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>

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
	WAV_HDR*			&hdr,				/* stores header info here (will be allocated) */
	short*				&leftPcm,			/* stores left (or mono) PCM channel here */
	short*				&rightPcm			/* stores right PCM channel (stereo only) here */
);

/* read_wave_header
 *  Parses the given file stream for the 44-byte WAV header.
 *
 *  Return value:
 *    Pointer to newly allocated WAV_HDR struct (be sure to delete later).
 */
WAV_HDR* read_wave_header(
	ifstream			&file				/* file stream to parse from (must be opened for reading) */
);


/* check_wave_header
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
	const WAV_HDR*		hdr,				/* pointer to header struct that's already been read */
	short*				&leftPcm,			/* stores left PCM channel here (will be allocated) */
	short*				&rightPcm			/* stores right PCM channel here (will be allocated for stereo files)*/
);

#endif //__WAVE_H_
=======
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
	WAV_HDR*			&hdr,				/* stores header info here (will be allocated) */
	short*				&leftPcm,			/* stores left (or mono) PCM channel here */
	short*				&rightPcm			/* stores right PCM channel (stereo only) here */
);

/* read_wave_header
 *  Parses the given file stream for the 44-byte WAV header.
 *
 *  Return value:
 *    Pointer to newly allocated WAV_HDR struct (be sure to delete later).
 */
WAV_HDR* read_wave_header(
	ifstream			&file				/* file stream to parse from (must be opened for reading) */
);


/* check_wave_header
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
	const WAV_HDR*		hdr,				/* pointer to header struct that's already been read */
	short*				&leftPcm,			/* stores left PCM channel here (will be allocated) */
	short*				&rightPcm			/* stores right PCM channel here (will be allocated for stereo files)*/
);

#endif //__WAVE_H_
>>>>>>> 044529c6491432a663cb8c0dbc9b00c37bb049e3
