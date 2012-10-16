#!/usr/bin/make -f

# Yes, it really is this simple.
PKG = $$(pkg-config --cflags --libs glib-2.0 libcurl)

SRC = perftest.c stats.c scenario.c
HDR = stats.h scenario.h

perftest: Makefile $(HDR) $(SRC)
	$(CC) -o $@ $(SRC) -std=c99 -g -O2 -Wall -Werror $(PKG) -luriparser -lm
