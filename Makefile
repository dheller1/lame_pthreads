all:
	g++ source/*.cpp -Wall -I/usr/local/include/lame -lpthread -lmp3lame -o lame_pthread
