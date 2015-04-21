# Makefile for local EVIO build

EVIO = evio-4.4.5
SRCDIR = $(EVIO)/src/libsrc
TAR = $(EVIO).tgz
WGET = $(shell which wget)
export EVIO_INSTALL_DIR = $(shell pwd)

all:	$(EVIO_INSTALL_DIR)/libevio.a

$(EVIO_INSTALL_DIR)/libevio.a: $(SRCDIR)/Makefile

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
	 https://coda.jlab.org/drupal/system/files/coda/evio/evio-4.4/$(EVIO).tgz)

clean:
	if [ -d $(SRCDIR) ]; then $(MAKE) -C $(SRCDIR) clean; touch -r $(TAR) $(SRCDIR); fi

realclean:
	if [ -d $(SRCDIR) ]; then $(MAKE) -C $(SRCDIR) realclean; fi
	rm -rf $(EVIO)

.PHONY: all srcdir clean realclean
