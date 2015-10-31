all: *.cc
	dbusxx-xml2cpp org.freedesktop.NetworkManager.Connection.Active.xml --adaptor=org.freedesktop.NetworkManager.Connection.Active-proxy.h
	clang++ *.cc -O0 -g3 -Wall -pedantic -std=c++14 -o laps2 `pkg-config --cflags --libs xcb alsa libudev dbus-c++-1`
