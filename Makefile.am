## Process this file with automake to produce Makefile.in

CC = gcc

LIBS = -lmodbus -lm -lconfig -lhashmap

bin_PROGRAMS = modio

modio_SOURCES = modio.c

modio_CPPFLAGS = -DREGISTER_PATH=\"$(modiodir)/\"

modio_LDADD = $(LIBS)

REGS = ./regs

modio_DATA = $(REGS)/adele_cbi2801224a.cfg \
	     $(REGS)/moxa_iologik_e1200.cfg

modiodir = $(datadir)/modio

