#include "lame_interface.h"

static pthread_mutex_t mutFilesFinished = PTHREAD_MUTEX_INITIALIZER;

int encode_to_file(lame_global_flags *gfp, const FMT_DATA *hdr, const short *leftPcm, const short *rightPcm,
	const int iDataSize, const char *filename)
{
	int numSamples = iDataSize / hdr->wBlockAlign;

	int mp3BufferSize = numSamples * 5 / 4 + 7200; // worst case estimate
	unsigned char *mp3Buffer = new unsigned char[mp3BufferSize];

	// call to lame_encode_buffer
	int mp3size = lame_encode_buffer(gfp, (short*)leftPcm, (short*)rightPcm, numSamples, mp3Buffer, mp3BufferSize);
	if (!(mp3size > 0)) {
		delete[] mp3Buffer;
		cerr << "No data was encoded by lame_encode_buffer. Return code: " << mp3size << endl;
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

#ifdef __VERBOSE_
	cout << "Wrote " << mp3size + flushSize << " bytes." << endl;
#endif
	return EXIT_SUCCESS;
}

void *complete_encode_worker(void* arg)
{
	int ret;
	ENC_WRK_ARGS *args = (ENC_WRK_ARGS*)arg; // parse argument struct

	while (true) {
#ifdef __VERBOSE_
		cout << "Checking for work\n";
#endif
		// determine which file to process next
		bool bFoundWork = false;
		int iFileIdx = -1;

		pthread_mutex_lock(&mutFilesFinished);
		for (int i = 0; i < args->iNumFiles; i++) {
			if (!args->pbFilesFinished[i]) {
				
				// NORMALLY: LOCK HERE
				args->pbFilesFinished[i] = true; // mark as being worked on
				// UNLOCK
				// but this is an atomic operation, so there shouldn't be any conflicts!
				
				iFileIdx = i;
				bFoundWork = true;
				break;
			}
		}
		pthread_mutex_unlock(&mutFilesFinished);

		if (!bFoundWork) {// done yet?
			return NULL; // break
		}
		string sMyFile = args->pFilenames->at(iFileIdx);
		string sMyFileOut = sMyFile.substr(0, sMyFile.length() - 3) + "mp3";

		// start working
		FMT_DATA *hdr = NULL;
		short *leftPcm = NULL, *rightPcm = NULL;
		// init encoding params
		lame_global_flags *gfp = lame_init();
		lame_set_brate(gfp, 192); // increase bitrate
		lame_set_quality(gfp, 3); // increase quality level
		lame_set_bWriteVbrTag(gfp, 0);

		// parse wave file
#ifdef __VERBOSE_
		printf("Parsing %s ...\n", sMyFile.c_str());
#endif
		int iDataSize = -1;
		ret = read_wave(sMyFile.c_str(), hdr, leftPcm, rightPcm, iDataSize);
		if (ret != EXIT_SUCCESS) {
			printf("Error in file %s. Skipping.\n", sMyFile.c_str());
			continue; // see if there's more to do
		}

		lame_set_num_channels(gfp, hdr->wChannels);
		lame_set_num_samples(gfp, iDataSize / hdr->wBlockAlign);
		// check params
		ret = lame_init_params(gfp);
		if (ret != 0) {
			cerr << "Invalid encoding parameters! Skipping file." << endl;
			continue;
		}

		// encode to mp3
		ret = encode_to_file(gfp, hdr, leftPcm, rightPcm, iDataSize, sMyFileOut.c_str());
		if (ret != EXIT_SUCCESS) {
			cerr << "Unable to encode mp3: " << sMyFileOut.c_str() << endl;
			continue;
		}

		printf("[:%i][ok] .... %s\n", args->iThreadId, sMyFile.c_str());
		++args->iProcessedFiles;

		lame_close(gfp);
		if (leftPcm != NULL) delete[] leftPcm;
		if (rightPcm != NULL) delete[] rightPcm;
		if (hdr != NULL) delete hdr;
	}

	pthread_exit((void*)0);
}
