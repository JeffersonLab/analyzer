# Makefile for local EVIO build

EVIO = evio-4.4.6
URL = https://coda.jlab.org/drupal/system/files
EVIOLIB = libevio.so
SRCDIR = $(EVIO)/src/libsrc
TAR = $(EVIO).tgz
WGET = $(shell which wget)
CURL = $(shell which curl)
export EVIOLIB
# For standalone build
ifndef EVIO_LIBDIR
  export EVIO_LIBDIR = $(shell pwd)
endif
ifndef EVIO_INCDIR
  export EVIO_INCDIR = $(shell pwd)
endif
ifndef SOFLAGS
  export SOFLAGS = -shared
endif

all:	$(EVIO_LIBDIR)/$(EVIOLIB)

$(EVIO_LIBDIR)/$(EVIOLIB): $(SRCDIR)/$(EVIOLIB)
	$(MAKE) -C $(SRCDIR) install

$(SRCDIR)/$(EVIOLIB): $(SRCDIR)/Makefile
	$(MAKE) -C $(SRCDIR)

$(SRCDIR)/Makefile:  $(SRCDIR) Makefile.libsrc
	cp -f Makefile.libsrc $(SRCDIR)/Makefile

$(SRCDIR): $(TAR)
	tar xf $(TAR)

$(TAR):
ifeq ($(strip $(WGET)),)
ifeq ($(strip $(CURL)),)
	$(error No EVIO tar ball and cannot find wget or curl to get it)
else
	$(shell $(CURL) $(URL)/$(EVIO).tgz -o $(EVIO).tgz)
endif
else
	$(shell $(WGET) --no-check-certificate $(URL)/$(EVIO).tgz)
endif

install: all
	cp -p evio.h $(ANALYZER)/include

clean:
	if [ -d $(SRCDIR) ]; then $(MAKE) -C $(SRCDIR) clean; fi

realclean:
	if [ -d $(SRCDIR) ]; then $(MAKE) -C $(SRCDIR) realclean; fi
	rm -rf $(EVIO)

.PHONY: all srcdir clean realclean
