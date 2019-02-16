CC=g++
CFLAGS=-c
LFLAGS=-lpthread
VERS=1.1

default: build clean

debug: CFLAGS += -g
debug: VERS := $(VERS)d
debug: default

compile:
	$(CC) $(CFLAGS) GEDS/*.cpp

build: compile
	$(CC) $(LFLAGS) -o GEDSv$(VERS) *.o

clean:
	rm -f *.o
