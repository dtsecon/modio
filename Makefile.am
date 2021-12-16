## Process this file with automake to produce Makefile.in

CC = gcc

#MODBUSIDIR = /usr/local/include/modbus

LIBS = -lmodbus -lm -lconfig -lhashmap

bin_PROGRAMS = modio

modio_SOURCES = modio.c

modio_CPPFLAGS =

modio_LDADD = $(LIBS)

#AM_CFLAGS = -I$(MODBUSIDIR)
