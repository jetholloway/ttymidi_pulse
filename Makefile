CXXFLAGS =
file=main

all: sample

debug: CXXFLAGS += -DDEBUG -g
debug: sample

sample:
	$(CXX) $(CXXFLAGS) $(file).cpp -o $(file) -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/ -ldbus-1 -Wall -Wextra -lglib-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0
