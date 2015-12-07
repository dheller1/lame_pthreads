          lame_pthreads README
==================================
 Multithreaded LAME mp3 encoding based on POSIX threads
  Dominik Heller - dominik.heller1@gmail.com
  November 20, 2015
 
 
  PREREQUISITES
==================================

  - LAME static library version 3.99.5
     http://lame.sourceforge.net/
     (Linux:   liblamemp3.a
      Windows: libmp3lame-static.lib
               libmpghip-static.lib)
              
  - Windows only:
    pthreads-win32 version 2.9.1 (pthreadVC2.lib or pthreadVC2.dll)
      https://www.sourceware.org/pthreads-win32/
    I've used the prebuilt DLL.
     
  - Windows only (standard header for Linux):
    dirent.h
      https://github.com/tronkko/dirent
       

  BUILD
==================================

  - Linux:
     I provided a (minimal) Makefile, so 
  	
  	   make
    
     is all you need. If anything goes wrong, ensure that the include path
     for "lame.h" is correct and the libraries libpthread and libmp3lame
     can be found.
     Feel free to change warning and optimization levels etc.
     Tested with: g++ 4.8.4
    
  - Windows:
     I provided a solution file for Microsoft Visual Studio 2013 (vc12).
     In the project settings, adjust the include paths and library paths so
     that LAME and pthreads-w32 headers and libs can be found. You can also
     use the DLL for pthreads (pthreadsVC2.dll). I only tested 32bit mode.
     While it is easier to build with Linux, I actually used  Windows for
     development so it's guaranteed to work. I didn't test MinGW or other
     compilers though.
     For a custom build, make sure that the preprocessor directive WIN32 has
     been set (should be default for Visual Studio at least).
     I've built the LAME static libs from source which was flawlessly working
     with VS2013.
     
   Note: It is presumed that sizeof(short) == 2, sizeof(int) == 4!

     
  USAGE
==================================

     ./lame_pthreads PATH [-nN]
   
   Program will look for WAV files in given folder PATH and convert to MP3.
   If -nN (e.g. -n8) is specified, N threads will be spawned for parallel
   processing of input files. Otherwise a default number of threads will be
   used.
   Non WAV-files will be ignored and invalid files ending in .wav should be
   skipped. Encoding time for all files is measured for an easy performance
   comparison with different numbers of threads.
   
   For a quick first impressions, I made some screenshots for Windows and
   Linux calls of the program.
   
   
  IMPLEMENTATION NOTES
==================================

   (1) My first idea was to use multiple threads to encode PCM data
   chunks of a single WAV-file in parallel, so that optimal performance
   would be granted even for a small number of files to be processed.
   While I got the chunk-based encoding to work sequentially, it turned
   out that the MP3 encoding process is not really suited for parallel
   processing in a single file without digging deep into the internals
   of LAME (e.g. doing psycho acoustics in a separate thread like LAME MT
   seemingly did), so I gave up on that approach after extensive testing
   and some additional research.
   (e.g. http://arstechnica.com/civis/viewtopic.php?f=6&t=50074)
   There are still some obsolete code fragments based on this approach
   which I decided to keep for your reference. They've been marked as
   obsolete in the respective header files.
   
   (2) The second approach was actually much easier and quicker to
   implement and represents the current state of the program:
   Use multithreading to encode several files in parallel, but only use
   a single thread for each file.
   As I was more accustomed to OpenMP parallelization until now, where
   the number of threads should be kept low (conforming to the number of
   CPU cores) to reduce overhead, I wanted to avoid spawning a thread
   for each individual file. Instead, each thread should dynamically
   fetch a yet-unprocessed file, mark it as being processed, encode it,
   and then continue working on the next file as long as more input files
   are available. This also provides a (very simple) load balancing
   when the number of threads corresponds to the number of cores.
   
   (3) When testing it actually turned out that the overhead for using
   many threads (#threads >> #cores) is quite low, so feel free to
   freely adjust the number of threads. Anyways, the parallel version
   with a low number of threads (e.g. -n4 on a quad-core) is evidently
   still much faster than the sequential version (-n1).
   
   (4) I've decided to stick to the style of LAME and pthreads and
   provide a lightweight non-OOP approach.
   
   (5) I've provided detailed documentation in the headers (wave.h,
   lame_interface.h) and sporadic comments in the source files.
   Check the headers first if you want to get a general overview
   of the code.
   The important main routines of the current implementation are:
      read_wave (wave.h)  -  Processes WAV input file (header and data)
      encode_to_file (lame_interface.h)
        Encodes a single input data stream to MP3 and writes output to file.
      complete_encode_worker (lame_interface.h)
        POSIX thread entry point / worker routine, does subsequent
        calls to encode_to_file to process several input files.
      
   
   
   LIMITATIONS & ISSUES
==================================

    I tried my best to come up with a clean, readable, and effective
    working solution, but please understand that I currently cannot further
    polish the code due to time limitations.
 
    I'll try to provide a list of issues that I'd continue to work on
    given the chance in order to improve stability:

    (1) While there is some error handling present to deal with invalid
    input files, this is by no means done perfectly. Output might become
    quite confusing if many invalid input files are present due to the
    parallel nature of the program.
    I've mixed cout/cerr/printf frequently as the stream-based output
    can become unreadable when other threads interfere.
	 
    (2) Memory allocation errors are not handled. There might even be
    some memory leaks, I didn't use profiling to find them yet.
    While I've tested processing hundreds of input files, I didn't test
    with very large amounts data (>100MB), so if you want to encode your
    whole audio CD collection I don't know if it works.
	 
    (3) The parallel access to the 'pbFilesProcessed' array, which determines
    which files are yet to be processed, is not protected by mutexes/locks.
    ---- This has been fixed as of commit 7c86063 ----
	 
    (4) WAV files can be quite complex and include all kinds of content,
    compression, etc. The program tries to exclude any incompatible files and
    it worked for the test files I was able to find, but I'm sure weird things
    can and will happen when trying to encode unusual WAV files.
    Some assumptions made in the program are:
      - first bytes of the file must be
        'RIFF'   char[4]
        fileLen  int
        'WAVE'   char[4]
      - data must be in PCM format (wFmtTag == 0x01)
      - number of channels must be 1 (Mono) or 2 (Stereo)
      - IFF chunks must be valid, we skip everything that's not 'fmt ' or 'data'
      - wBlockAlign == wBitsPerSample * wChannels / 8

    (5) I've mixed hungarian notation and arbitrary naming inconsistently.
