# Makefile for "hana" (C++ Hall A Analyzer) Event Decoder.
# R. Michaels, rom@jlab.org, Mar 2001
#
# Test Executibles:
# tstcoda  --  test of abstract interface to THaCodaFile and THaEtClient.
# tstio    --  simple tests of CODA I/O from a file.
# tdecpr   --  test of decoder with self-explanatory printouts.
# tdecex   --  test of decoder, example for a use by a detector class.
# etclient --  test of ET connection for online data.
# prfact   --  standalone code to print the prescale factors and exit.  
# epicsd   --  test of EPICS data
# 
# To understand how to use decoding classes, look at the 'main'
# routines tstcoda_main.C, tstio_main.C, tdecpr_main.C, tdecex_main.C etc
# corresponding to the above executibles.  Also see haGenDetTest.C
# which shows proposed usage in a 'generic' detector class.

# Note regarding software from DAQ group
# evio.h, evio.C, swap_util.C, et.h are taken from DAQ group intact
# (only a few tiny mod's by me).  For this reason I did not rename 
# those modules with prefix "THa".  Next, swapped_intcpy.c is a version
# extracted from swap_util and modified by Ole for higher speed.

# name and version of this release
VERS = hana_decode_1.6

# If one defines STANDALONE, the library and test execs are compiled 
# independently of the rest of the analyzer.  Normally this is commented out.
# export STANDALONE = 1

# MAINDIR is the source of headers for main code.
# Also need objects from MAINDIR for standalone test executibles,
# but NOT for making library.
export MAINDIR = ../src

# Use flag ONLINE_ET below if compiling standalone online code (ET system).
# This flag gets passed by the main Makefile if not compiling standalone tests.
# CODA environment variable must be set.  Examples are
#   CODA = /adaqfs/coda/2.2        (in adaq cluster)
#   CODA = /data7/user/coda/2.2    (on haplix cluster)
# User's $LD_LIBRARY_PATH must include $CODA/$OSNAME/lib  (OSNAME = SunOS or Linux) 
# Example:  OSNAME=Linux ; export OSNAME 
# LD_LIBRARY_PATH=/app/root/lib:$CODA/$OSNAME/lib:.   ; export LD_LIBRARY_PATH
# export ONLINE_ET = 1

# Use this if profiling (note: it slows down the code)
# export PROFILE = 1

# export DEBUG = 1


ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLIBS     := $(shell root-config --libs)
ROOTGLIBS    := $(shell root-config --glibs)

INCLUDES      = $(ROOTCFLAGS) 

ifndef STANDALONE
   INCLUDES += -I$(MAINDIR)
else
   ARCH          = linuxegcs
  # ARCH          = solarisCC5

# Only define compiler options for STANDALONE, otherwise inherit them from
# top-level Makefile

  DEFINES = -DSTANDALONE
  ifeq ($(ARCH),solarisCC5)
     DEFINES      += -DSUNVERS
     # Solaris CC 5.0
     CXX           = CC
     LD            = CC
     SOFLAGS       = -G
     ifdef DEBUG
        CXXFLG      = -g -KPIC
        LDFLAGS     = -g
     else
        CXXFLG      = -O -KPIC
        LDFLAGS     = -O
     endif
  endif

  ifeq ($(ARCH),linuxegcs)
     DEFINES      += -DLINUXVERS
     # Linux with egcs (>= RedHat 5.2)
     CXX           = g++
     LD            = g++
     SOFLAGS       = -shared
     ifdef DEBUG
        CXXFLG      = -g -O0
        LDFLAGS     = -g -O0
     else
        CXXFLG      = -O2
        LDFLAGS     = -O2
     endif
     CXXFLG        += -Wall -fPIC
     CXXEXTFLG      = -Woverloaded-virtual
  endif
  ifdef WITH_DEBUG
     DEFINES     += -DWITH_DEBUG
  endif
#  CXXFLG       += $(INCLUDES) $(DEFINES)
endif

ifeq ($(CXX),)
   $(error $(ARCH) invalid architecture)
endif

MAKEDEPEND    = g++

export OSNAME := $(shell uname)

ifeq ($(OSNAME),SunOS)

   EVIO_LIB=libevio_sun.a
   GLIB =  -lm -lc -lgen -lw -lnsl -ldl
   SLIB = -L/opt/SUNWspro/SC4.2/lib -lF77 -lM77 -lsunmath
# all this needed for ET...  up to comment below
   ET_AC_FLAGS = -D_REENTRANT -D_POSIX_THREAD_SEMANTICS
   ET_CFLAGS = -mt -fast -xO5 -KPIC $(ET_AC_FLAGS) -DSUNVERS
# CODA environment variable must be set.  Examples are
#   CODA = /adaqfs/coda/2.2        (in adaq cluster)
#   CODA = /data7/user/coda/2.2    (on haplix cluster)
   LIBET = $(CODA)/SunOS/lib/libet.so
   ONLIBS = $(LIBET) -lposix4 -lnsl -lsocket -lresolv
   LIBS = $(EVIO_LIB) $(GLIB)
   ALL_LIBS = $(LIBS) 

endif

ifeq ($(OSNAME),Linux)

   LIBS          =
   GLIBS         = -L/usr/X11R6/lib -lXpm -lX11
   EVIO_LIB      = libevio_linux.a
   ALL_LIBS      = $(EVIO_LIB) $(GLIBS) $(ROOTLIBS) $(ROOTGLIBS)

# ONLIBS is needed for ET
   ET_AC_FLAGS = -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS
   ET_CFLAGS = -02 -fPIC -I. $(ET_AC_FLAGS) -DLINUXVERS
# CODA environment variable must be set.  Examples are
#   CODA = /adaqfs/coda/2.2        (in adaq cluster)
#   CODA = /data7/user/coda/2.2    (on haplix cluster)
   LIBET = $(CODA)/Linux/lib/libet.so
   ONLIBS = $(LIBET) -lieee -lpthread -ldl -lresolv

   ifdef PROFILE
      CXXFLG   += -pg
   endif

endif

# ... finished, what ET needs.
ifdef ONLINE_ET
  DC_DEFINES += -DONLINE_ET
  ALL_LIBS   += $(ONLIBS)
endif

DEFINES      += $(DC_DEFINES)
CFLAGS        = $(CXXFLG) $(INCLUDES) $(DEFINES)
CXXFLAGS      = $(CXXFLG) $(CXXEXTFLG) $(INCLUDES) $(DEFINES)
LIBS         += $(ROOTLIBS) $(SYSLIBS)
GLIBS        += $(ROOTGLIBS) $(SYSLIBS)

#-----------------------------------------------------------------------
# DECODE_OBJS is the package for CODA I/O and decoder written by R. Michaels.
# This, together with libevio, is what other developers need.

SRC = THaUsrstrutils.C THaCrateMap.C THaCodaData.C THaHelicity.C \
      THaEpics.C THaFastBusWord.C THaCodaFile.C THaSlotData.C \
      THaEvData.C evio.C THaCodaDecoder.C

PROGS = tstio tdecpr tdecex prfact epicsd 
# If you want to use the ET system at Jlab.
ifdef ONLINE_ET
  SRC += THaEtClient.C
  PROGS += etclient
  PROGS += tstcoda
endif

RCHEAD := $(SRC:.C=.h) THaBenchmark.h
HEAD = $(RCHEAD)
ifdef ONLINE_ET
  HEAD += et.h
endif
# sources without headers:
SRC += swap_util.C swapped_intcpy.c
DEPS = $(patsubst %.c,%.d,$(SRC:.C=.d))
DECODE_OBJS = $(patsubst %.c,%.o,$(SRC:%.C=%.o)) THaDecDict.o

ifndef STANDALONE
  all:  $(LIBDIR)/libdc.so.$(VERSION)
else
  all:  libdc.a
endif

# To compile local test codes:  Uncomment "export STANDALONE = 1" at top of Makefile
# Normally this should be commented out.

ifdef STANDALONE
  all: $(PROGS) libdc.a
endif

# Library for main analyzer

libdc.a: $(DECODE_OBJS)
	rm -f $@
	ar crs $@ $^
ifndef STANDALONE
	cp -p $@ $(LIBDIR)
endif

libdc.so: $(DECODE_OBJS)
	rm -f $@
ifeq ($(strip $(SONAME)),)
	$(LD) $(LDFLAGS) $(SOFLAGS) -o $@ $^
else
	$(LD) $(LDFLAGS) $(SOFLAGS) $(SONAME)libdc.so.$(SOVERSION) -o $@ $^
endif

ifndef STANDALONE
$(LIBDIR)/libdc.so.$(VERSION): libdc.so
	rm -f $@ $(LIBDIR)/libdc.so.$(SOVERSION)
	cp -p $< $@
endif

# Test programs for standalone tests of decoder.
# Need a few objects from MAINDIR because of some links to those classes.
ifdef STANDALONE
   MAINOBJS = 
else
   MAINOBJS = $(MAINDIR)/libHallA.so
endif

tstio: tstio_main.o $(DECODE_OBJS) $(SRC) $(EVIO_LIB) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ tstio_main.o $(DECODE_OBJS) $(ALL_LIBS) $(MAINOBJS)

tdecpr: $(DECODE_OBJS) $(SRC) tdecpr_main.o $(HEAD) $(SRC) $(EVIO_LIB) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ tdecpr_main.o $(DECODE_OBJS) $(ALL_LIBS) $(MAINOBJS)

tdecex: $(DECODE_OBJS) $(SRC) tdecex_main.o THaGenDetTest.o $(HEAD) THaGenDetTest.h $(SRC) $(DECODE_OBJS) $(EVIO_LIBS) $(MAINOBJS)  
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ tdecex_main.o THaGenDetTest.o $(DECODE_OBJS) $(ALL_LIBS) $(MAINOBJS)

etclient: etclient_main.o $(DECODE_OBJS) $(MAINOBJS)
	$(CXX) $(CXXFLAGS) -o $@ etclient_main.o $(DECODE_OBJS) $(ALL_LIBS) $(MAINOBJS)

prfact: $(DECODE_OBJS) $(SRC) prfact_main.o $(HEAD) $(EVIO_LIB) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ prfact_main.o $(DECODE_OBJS) $(ALL_LIBS) $(MAINOBJS)

epicsd: $(DECODE_OBJS) $(SRC) epics_main.o $(HEAD) $(EVIO_LIB) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ epics_main.o $(DECODE_OBJS) $(ALL_LIBS) $(MAINOBJS)

tstcoda: tstcoda_main.o THaCodaFile.o THaEtClient.o THaCodaData.o THaCodaFile.h THaEtClient.h THaCodaData.h $(DECODE_OBJS) $(MAINOBJS)
	$(CXX) $(CXXFLAGS) -o $@ tstcoda_main.o $(DECODE_OBJS) $(ALL_LIBS) $(MAINOBJS)


# Below is the evio library, which comes rather directly 
# from CODA group with minor tweaking by R. Michaels & O. Hansen.

libevio_sun.a:  evio.o swap_util.o swapped_intcpy.o
	rm -f $@
	ar cr $@ evio.o swap_util.o swapped_intcpy.o

libevio_linux.a:  evio.o swap_util.o swapped_intcpy.o
	rm -f $@
	ar cr $@ evio.o swap_util.o swapped_intcpy.o

#libevio_hpux.a:  evio.o swap_util.o swapped_intcpy.o
#	rm -f $@
#	ar cr $@ evio.o swap_util.o swapped_intcpy.o

#evio.o: evio.C
#	$(CXX) -c  $<

# swap_util.o: swap_util.C
# 	$(CXX) -c  $<

swapped_intcpy.o: swapped_intcpy.c
	$(CC) $(CFLAGS) -c  $<

# Dictionary

THaDecDict.C: $(RCHEAD) haDecode_LinkDef.h
	@echo "Generating Decoder Dictionary..."
	$(ROOTSYS)/bin/rootcint -f THaDecDict.C -c $(CXXFLAGS) $^

# create a tar file of ./$(VERS)/* (all code)

tarfile: clean version
	tar cvf $(VERS).tar ./hana_decode

version:
	mkdir $(VERS) 
	cp *.c ./$(VERS) 
	cp *.C ./$(VERS) 
	cp *.h ./$(VERS) 
	cp Makefile ./$(VERS) 
	cp snippet.dat ./$(VERS)
	cp README* ./$(VERS)
	cp ChangeLog ./$(VERS)
	tar cvf $(VERS).tar ./$(VERS)

clean:  clean_evio
	rm -f *.o *.a core *~ THaDecDict* *.out *.tar $(PROGS)

realclean:  clean
	rm -f *.d

clean_evio:
	rm -f evio.o swap_util.o swapped_intcpy.o

install:	all
	mkdir -p $(ANALYZER)/src/hana_decode
	cp -pu $(SRC) $(HEAD) haDecode_LinkDef.h Makefile $(ANALYZER)/src/hana_decode
	cp -pu $(HEAD) $(ANALYZER)/include
#	-cp -puvf libdc.so $(ANALYZER)/$(PLATFORM)
###

.SUFFIXES:
.SUFFIXES: .c .cc .cpp .C .o .d

%.o:	%.C
	$(CXX) $(CXXFLAGS) -c $<

%.d:	%.C
	@echo Creating dependencies for $<
#	@$(SHELL) -ec '$(CXX) -MM $(CXXFLAGS) -c $< \
#		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
#		[ -s $@ ] || rm -f $@'
	@$(SHELL) -ec '$(MAKEDEPEND) -MM $(INCLUDES) $(DEFINES) -c $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

-include $(DEPS)
