# Makefile for local EVIO build
# Caller must have set EVIO_LIBDIR and EVIO_INCDIR install locations

EVIO = evio-4.4.5
SUBDIR = $(EVIO)/src/libsrc
TAR = $(EVIO).tgz
WGET = $(shell which wget)

all:	$(TAR)
	tar xf $(TAR)
	cp -f Makefile.libsrc $(EVIO)/src/libsrc/Makefile
	make -C $(SUBDIR) install

$(TAR):
ifeq ($(strip $(WGET)),)
	$(error No EVIO tar ball and cannot find wget to get it)
endif
	$(shell $(WGET) --no-check-certificate https://coda.jlab.org/drupal/system/files/coda/evio/evio-4.4/$(EVIO).tgz)

clean:
	rm -rf $(EVIO)

.PHONY: all
