# src/Makefile.am

CC = gcc

LIBS = -lmodbus -lm -lconfig -lhashmap

bin_PROGRAMS = modio

modio_SOURCES = modio.c

modio_CPPFLAGS = -DREGISTER_PATH=\"$(modiodir)/\"

modio_CFLAGS = -Werror

modio_LDADD = $(LIBS)
