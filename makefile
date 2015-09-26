all: *.cc
	clang++ *.cc -O0 -g3 -Wall -pedantic -std=c++14 -o laps2 `pkg-config --cflags --libs xcb alsa libudev`
