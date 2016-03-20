# -*- Mode: makefile-gmake -*-
# Simple Makefile for C API only libevio.so

LIB = $(EVIOLIB)
SRC = evio.c eviofmt.c eviofmtswap.c evioswap.c
HDR = evio.h
OBJ = $(SRC:.c=.o)

CC = gcc
CCFLAGS = -O3 -fPIC -I.

ifndef EVIO_LIBDIR
  $(error $$EVIO_LIBDIR not defined)
endif
ifndef EVIO_INCDIR
  $(error $$EVIO_INCDIR not defined)
endif

all:	$(LIB)

$(LIB): $(OBJ)
	$(CC) $(LDFLAGS) $(SOFLAGS) -o $@ $(OBJ)

%.o:	%.c $(HDR)
	$(CC) $(CCFLAGS) -o $@ -c $<

install: $(addprefix $(EVIO_LIBDIR)/,$(LIB)) $(addprefix $(EVIO_INCDIR)/,$(HDR))

uninstall:
	rm -f $(addprefix $(EVIO_LIBDIR)/,$(LIB))
	rm -f $(addprefix $(EVIO_INCDIR)/,$(HDR))

clean:
	rm -f *.o $(LIB)

realclean:	clean uninstall

$(EVIO_LIBDIR)/$(LIB):	$(LIB)
	@mkdir -p $(EVIO_LIBDIR)
	cp -p $(LIB) $(EVIO_LIBDIR)

$(EVIO_INCDIR)/%.h:	%.h
	@mkdir -p $(EVIO_INCDIR)
	cp -p $< $(EVIO_INCDIR)

.SUFFIXES:

.PHONY: all install uninstall clean realclean
