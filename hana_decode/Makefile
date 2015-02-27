# Makefile for "hana" (C++ Hall A Analyzer) Event Decoder.
# R. Michaels, rom@jlab.org, Mar 2001
#
# Test Executables:
# tstoo    --  tests of OO decoder
# tstfadc  --  tests of FADC 250 class
# tstskel  --  test of SkeltonModule
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
# corresponding to the above executables.  Also see haGenDetTest.C
# which shows proposed usage in a 'generic' detector class.

# Note regarding software from DAQ group:
# EVIO headers and libraries must be installed in $EVIO_INCDIR and
# $EVIO_LIBDIR, respectively.

# name and version of this release
VERS = hana_decode_1.6

# If one defines STANDALONE, the library and test execs are compiled
# independently of the rest of the analyzer.  Normally this is commented out.
# export STANDALONE = 1

# MAINDIR is the source of headers for main code.
# Also need objects from MAINDIR for standalone test executables,
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
ROOTBIN      := $(shell root-config --bindir)
ROOTINC      := -I$(shell root-config --incdir)
CXX          := $(shell root-config --cxx)
CC           := $(shell root-config --cc)

INCLUDES =

ifndef STANDALONE
   INCLUDES += -I$(MAINDIR)
else
   ARCH          = linux
  # ARCH           = macosx
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
        DEFINES    += -DNDEBUG
     endif
  endif

  ifeq ($(ARCH),linux)
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
        DEFINES    += -DNDEBUG
     endif
     CXXFLG        += -Wall -fPIC
     CXXEXTFLG      = -Woverloaded-virtual
  endif

  ifeq ($(ARCH),macosx)
     # EXPERIMENTAL: Mac OS X with Xcode 3.x/gcc 4.x
     DEFINES      += -DMACVERS
     CXX           = g++
     LD            = g++
     SOFLAGS       = -shared -Wl,-undefined,dynamic_lookup
     SONAME        = -Wl,-install_name,@rpath/
     ifdef DEBUG
        CXXFLG      = -g -O0
        LDFLAGS     = -g -O0
     else
        CXXFLG      = -O2
        LDFLAGS     = -O2
        DEFINES    += -DNDEBUG
     endif
     CXXFLG        += -Wall -fPIC
     CXXEXTFLG      = -Woverloaded-virtual
  endif

  ifdef WITH_DEBUG
     DEFINES     += -DWITH_DEBUG
  endif
  DEFINES      += $(shell getconf LFS_CFLAGS)
endif

MAKEDEPEND    = g++

export OSNAME := $(shell uname)

EVIO_LIB      = -L$(EVIO_LIBDIR) -levio

ifeq ($(OSNAME),SunOS)

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
   LIBS = $(GLIB)
   ALL_LIBS = $(LIBS)

endif

ifeq ($(OSNAME),Linux)

   LIBS          =
   GLIBS         = -L/usr/X11R6/lib -lXpm -lX11
   ALL_LIBS      = $(GLIBS) $(ROOTLIBS) $(ROOTGLIBS)

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

ifeq ($(OSNAME),Darwin)
  ALL_LIBS = $(ROOTLIBS) $(ROOTGLIBS)
endif

# ... finished, what ET needs.
ifdef ONLINE_ET
  DC_DEFINES += -DONLINE_ET
  ALL_LIBS   += $(ONLIBS)
endif

# External EVIO support
INCLUDES     += -I$(EVIO_INCDIR)

DEFINES      += $(DC_DEFINES)
CFLAGS        = $(CXXFLG) $(ROOTCLAGS) $(INCLUDES) $(DEFINES)
CXXFLAGS      = $(CXXFLG) $(CXXEXTFLG) $(ROOTCFLAGS) $(INCLUDES) $(DEFINES)
LIBS         += $(ROOTLIBS) $(SYSLIBS)
GLIBS        += $(ROOTGLIBS) $(SYSLIBS)

#-----------------------------------------------------------------------
# DECODE_OBJS is the package for CODA I/O and decoder written by R. Michaels.
# This, together with libevio, is what other developers need.

SRC = THaUsrstrutils.C THaCrateMap.C THaCodaData.C \
      THaEpics.C THaFastBusWord.C THaCodaFile.C THaSlotData.C \
      THaEvData.C THaCodaDecoder.C SimDecoder.C \
      CodaDecoder.C Module.C VmeModule.C FastbusModule.C  \
      Lecroy1877Module.C Lecroy1881Module.C Lecroy1875Module.C \
      Fadc250Module.C GenScaler.C Scaler560.C Scaler1151.C \
      Scaler3800.C Scaler3801.C F1TDCModule.C SkeletonModule.C

PROGS = tstoo tstfadc tstskel tstio tdecpr tdecex prfact epicsd
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

static:	libdc.a

libdc.so: $(DECODE_OBJS)
	rm -f $@
ifeq ($(strip $(SONAME)),)
	$(LD) $(LDFLAGS) $(SOFLAGS) -o $@ $^ $(EVIO_LIB)
else
	$(LD) $(LDFLAGS) $(SOFLAGS) $(SONAME)libdc.so.$(SOVERSION) -o $@ $^ $(EVIO_LIB)
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
   MAINOBJS = ../libHallA.so ../libscaler.so
endif

tstoo: tstoo_main.o $(DECODE_OBJS) $(SRC)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ tstoo_main.o $(DECODE_OBJS) $(ALL_LIBS) $(EVIO_LIB) $(MAINOBJS)

tstfadc: tstfadc_main.o $(DECODE_OBJS) $(SRC) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ tstfadc_main.o $(DECODE_OBJS) $(ALL_LIBS) $(EVIO_LIB) $(MAINOBJS)

tstskel: tstskel_main.o $(DECODE_OBJS) $(SRC) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ tstskel_main.o $(DECODE_OBJS) $(ALL_LIBS) $(EVIO_LIB) $(MAINOBJS)

tstio: tstio_main.o $(DECODE_OBJS) $(SRC) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ tstio_main.o $(DECODE_OBJS) $(ALL_LIBS) $(EVIO_LIB) $(MAINOBJS)

tdecpr: $(DECODE_OBJS) $(SRC) tdecpr_main.o $(HEAD) $(SRC) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ tdecpr_main.o $(DECODE_OBJS) $(ALL_LIBS) $(EVIO_LIB) $(MAINOBJS)

tdecex: $(DECODE_OBJS) $(SRC) tdecex_main.o THaGenDetTest.o $(HEAD) THaGenDetTest.h $(SRC) $(DECODE_OBJS) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ tdecex_main.o THaGenDetTest.o $(DECODE_OBJS) $(ALL_LIBS) $(EVIO_LIB) $(MAINOBJS)

etclient: etclient_main.o $(DECODE_OBJS) $(MAINOBJS)
	$(CXX) $(CXXFLAGS) -o $@ etclient_main.o $(DECODE_OBJS) $(ALL_LIBS) $(EVIO_LIB) $(MAINOBJS)

prfact: $(DECODE_OBJS) $(SRC) prfact_main.o $(HEAD) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ prfact_main.o $(DECODE_OBJS) $(EVIO_LIB) $(MAINOBJS)

epicsd: $(DECODE_OBJS) $(SRC) epics_main.o $(HEAD) $(MAINOBJS)
	rm -f $@
	$(CXX) $(CXXFLAGS) -o $@ epics_main.o $(DECODE_OBJS) $(ALL_LIBS) $(EVIO_LIB) $(MAINOBJS)

tstcoda: tstcoda_main.o THaCodaFile.o THaEtClient.o THaCodaData.o THaCodaFile.h THaEtClient.h THaCodaData.h $(DECODE_OBJS) $(MAINOBJS)
	$(CXX) $(CXXFLAGS) -o $@ tstcoda_main.o $(DECODE_OBJS) $(ALL_LIBS) $(EVIO_LIB) $(MAINOBJS)


# Dictionary

THaDecDict.C: $(RCHEAD) haDecode_LinkDef.h
	@echo "Generating Decoder Dictionary..."
	$(ROOTBIN)/rootcint -f THaDecDict.C -c $(ROOTINC) $(INCLUDES) $(DEFINES) $^

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

clean:
	rm -f *.{a,out,o,os} *.so.* core *~ THaDecDict* $(VERS).tar $(PROGS)

realclean:  clean
	rm -f *.d

install:	all
	mkdir -p $(ANALYZER)/src/hana_decode
	cp -pu $(SRC) $(HEAD) haDecode_LinkDef.h Makefile $(ANALYZER)/src/hana_decode
	cp -pu $(HEAD) $(ANALYZER)/include

.SUFFIXES:
.SUFFIXES: .c .cc .cpp .C .o .os .d

%.o:	%.C
	$(CXX) $(CXXFLAGS) -c $<

%.d:	%.C
	@echo Creating dependencies for $<
#	@$(SHELL) -ec '$(CXX) -MM $(CXXFLAGS) -c $< \
#		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
#		[ -s $@ ] || rm -f $@'
	@$(SHELL) -ec '$(MAKEDEPEND) -MM $(ROOTINC) $(INCLUDES) $(DEFINES) -c $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

-include $(DEPS)
