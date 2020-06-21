CC=g++
CFLAGS=-c -Isentry/include
LFLAGS=-pthread -lsentry -Lout
VERS=1.1

default: sentry build clean

debug: CFLAGS += -g
debug: VERS := $(VERS)d
debug: default

sentry:
	cmake -S sentry -B out
	cmake --build out

compile:
	$(CC) $(CFLAGS) GEDS/*.cpp

build: compile
	$(CC) $(LFLAGS) -o GEDSv$(VERS) *.o

clean:
	rm -f *.o
