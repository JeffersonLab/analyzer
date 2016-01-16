# Makefile for local EVIO build

EVIO = evio-4.4.6
EVIOLIB = libevio.so
SRCDIR = $(EVIO)/src/libsrc
TAR = $(EVIO).tgz
WGET = $(shell which wget)
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

$(EVIO_LIBDIR)/$(EVIOLIB): $(SRCDIR)/Makefile

$(SRCDIR)/Makefile:  $(SRCDIR) Makefile.libsrc
	cp -f Makefile.libsrc $(SRCDIR)/Makefile
	$(MAKE) -C $(SRCDIR) install

$(SRCDIR): $(TAR)
	tar xf $(TAR)

$(TAR):
ifeq ($(strip $(WGET)),)
	$(error No EVIO tar ball and cannot find wget to get it)
endif
	$(shell $(WGET) --no-check-certificate \
	 https://coda.jlab.org/drupal/system/files/$(EVIO).tgz)

clean:
	if [ -d $(SRCDIR) ]; then $(MAKE) -C $(SRCDIR) clean; touch -r $(TAR) $(SRCDIR); fi

realclean:
	if [ -d $(SRCDIR) ]; then $(MAKE) -C $(SRCDIR) realclean; fi
	rm -rf $(EVIO)

.PHONY: all srcdir clean realclean
