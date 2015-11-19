#include <iostream>
#include <list>
#include <string>
#include <vector>
#include "dirent.h"

#include "lame_interface.h"

using namespace std;

bool string_ends_with(const string &fullString, const string &subString)
{
	if (subString.length() > fullString.length()) return false;
	else {
		// lowercase conversion
		string fullString_l = fullString;
		for (unsigned int i = 0; i < fullString_l.length(); i++) {
			if ('A' <= fullString_l[i] && fullString_l[i] <= 'Z')
				fullString_l[i] = fullString_l[i] - ('Z' - 'z');
		}

		string subString_l = subString;
		for (unsigned int i = 0; i < subString_l.length(); i++) {
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

int main(void)
{
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