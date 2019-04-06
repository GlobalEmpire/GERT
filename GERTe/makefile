CC=g++
CFLAGS=-pthread
VERS=1.1

default: build

debug: CFLAGS += -g -rdynamic
debug: VERS := $(VERS)d
debug: default

build:
	$(CC) $(CFLAGS) GEDS/*.cpp -o GEDSv$(VERS)
