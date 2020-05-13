CC=g++
CFLAGS=-c
LFLAGS=-pthread
VERS=1.1

default: build clean

debug: CFLAGS += -g -rdynamic
debug: VERS := $(VERS)d
debug: default

compile:
	$(CC) $(CFLAGS) GEDS/*.cpp

build: compile
	$(CC) $(LFLAGS) -o GEDSv$(VERS) *.o

clean:
	rm -f *.o
