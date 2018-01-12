CC = g++
CFLAGS = -Wall -O2
DEBUGFLAGS = -g

INCLUDES = include/*.h
SRC = src/*.cpp

LIBS = -ljsoncpp

OUTPUT = notepadserver

all: release

release:
	$(CC) $(CFLAGS) -I$(INCLUDES) $(LIBS) $(SRC) -o build/$(OUTPUT)

debug:
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -I$(INCLUDES) $(LIBS) $(SRC) -o build/$(OUTPUT)-debug

clean:
	rm -rf build/*
