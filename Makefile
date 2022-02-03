####
# Makefile for nifty clean

# Compiler flags
CC ?= cc -pipe

COPT = -O3
CDEBUG = -g -O -Wall
#CFLAGS = $(CDEBUG)
CFLAGS ?= $(COPT) -Wno-stringop-overflow

# source files
SRCS = match.c parse_rc.c traverse.c utilities.c main.c
HDRS = niftyclean.h
OBJS = match.o parse_rc.o traverse.o utilities.o main.o

all: niftyclean

# renamed target 
niftyclean: $(OBJS)
	$(CC) $(CFLAGS) -o clean $(OBJS) $(LIBS)


$(OBJS): $(HDRS)

####
# standard pseudotargets

lint::
	lint -b $(SRCS)

clean:: distclean
	@echo Use \"make all\" or \"make niftyclean\" to build the program.

distclean::
	rm -f clean *.o core

####
# Set this to be a BSD-compatible install program.
INSTALL ?= install
# installation prefix
PREFIX ?= /usr/local

install: niftyclean
	$(INSTALL) -s clean $(PREFIX)/bin
	$(INSTALL) clean.1 $(PREFIX)/man/man1
