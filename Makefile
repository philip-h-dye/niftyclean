# Makefile for nifty clean

# installation prefix
PREFIX ?= /usr/local

# Compiler flags
CC ?= cc -pipe
COPT = -O3
# debugging...
CDEBUG = -g -O -Wall

# DEFS = -DVICE

CFLAGS ?= $(COPT)

# source files
SRCS = match.c parse_rc.c traverse.c utilities.c main.c
HDRS = niftyclean.h
OBJS = match.o parse_rc.o traverse.o utilities.o main.o

all:: niftyclean

# renamed target 
niftyclean: $(OBJS)
	$(CC) $(CFLAGS) -o clean $(OBJS) $(LIBS)


$(OBJS): $(HDRS)

# standard pseudotargets

lint::
	lint -b $(SRCS)

clean::
	@ echo ""
	@ echo Use \"make distclean\" or \"make niftyclean\", depending.
	@ echo ""

distclean::
	rm -f clean *.o *.u core

install: clean
	$(INSTALL) -s clean $(PREFIX)/bin
	$(INSTALL) clean.1 $(PREFIX)/man/man1
