# Makefile for nifty clean

# installation prefix
PREFIX ?= /usr/local

# Compiler flags
CC ?= cc -pipe
COPT = -O3
#debugging...
CDEBUG = -g -O -Wall

# DEFS = -DVICE

CFLAGS ?= $(CDEBUG)

# source files
SRCS = match.c parse_rc.c traverse.c utilities.c main.c
HDRS = niftyclean.h
OBJS = match.o parse_rc.o traverse.o utilities.o main.o

all:: clean

clean: $(OBJS)
	$(CC) $(CFLAGS) -o clean $(OBJS) $(LIBS)


$(OBJS): $(HDRS)

# standard pseudotargets

lint::
	lint -b $(SRCS)

distclean::
	rm -f clean *.o *.u core

install: clean
	- mkdir $(PREFIX)bin
	$(INSTALL) -s clean $(PREFIX)/bin

installdoc: clean.application
	- mkdir $(PREFIX)help
	$(INSTALL) -c -m 644 clean.application $(PREFIX)/help
