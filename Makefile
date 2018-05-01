#------------------------------------------------------------------------------

# Compile extra debugging code (slight performance impact)
export WITH_DEBUG = 1

# Compile debug version
#export DEBUG = 1

# Include libraries for reading from the ET ring
#  (only for adaq? machines with the Coda libraries )
#export ONLINE_ET = 1
#------------------------------------------------------------------------------

# SOVERSION should be numerical only - it becomes the shared lib soversion
# EXTVERS (optional) describes the build, e.g. "dbg", "et", "gcc33" etc.
SOVERSION  := 1.7
PATCH   := 0
VERSION := $(SOVERSION).$(PATCH)
EXTVERS := -devel
#EXTVERS := -et
NAME    := analyzer-$(VERSION)
VERCODE := $(shell echo $(subst ., ,$(SOVERSION)) $(PATCH) | \
   awk '{ print $$1*65536 + $$2*256 + $$3 }')

#------------------------------------------------------------------------------

MACHINE := $(shell uname -s)
ARCH    := linux
SOSUF   := so
ifeq ($(MACHINE),Darwin)
  ARCH := macosx
  SOSUF := dylib
endif

ifndef PLATFORM
PLATFORM = bin
endif

ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLIBS     := $(shell root-config --libs)
ROOTGLIBS    := $(shell root-config --glibs)
ROOTBIN      := $(shell root-config --bindir)
ROOTINC      := -I$(shell root-config --incdir)
CXX          := $(shell root-config --cxx)
CC           := $(shell root-config --cc)

HA_DIR       := $(shell pwd)
DCDIR        := hana_decode
LIBDIR       := $(shell pwd)
HALLALIBS    := -L$(LIBDIR) -lHallA -ldc 
SUBDIRS      := $(DCDIR) 
INCDIRS      := $(addprefix $(HA_DIR)/, src $(SUBDIRS))
HA_DICT      := haDict

LIBS         :=
GLIBS        :=

INCLUDES     := $(addprefix -I, $(INCDIRS) )

HAVE_EVIO    = no
ifneq ($(EVIO_LIBDIR),)
ifneq ($(EVIO_INCDIR),)
ifneq ($(wildcard $(EVIO_INCDIR)/evio.h),)
ifneq ($(wildcard $(EVIO_LIBDIR)/libevio.*),)
  HAVE_EVIO = yes
endif
endif
endif
endif
ifeq ($(HAVE_EVIO),no)
EVIO_ARCH    := $(shell uname -s)-$(shell uname -m)
ifdef EVIO
  ifneq ($(wildcard $(EVIO)/$(EVIO_ARCH)/include/evio.h),)
  ifneq ($(wildcard $(EVIO)/$(EVIO_ARCH)/lib/libevio.*),)
    export EVIO_INCDIR := $(EVIO)/$(EVIO_ARCH)/include
    export EVIO_LIBDIR := $(EVIO)/$(EVIO_ARCH)/lib
    HAVE_EVIO = yes
endif
  endif
else ifdef CODA
  ifneq ($(wildcard $(CODA)/$(EVIO_ARCH)/include/evio.h),)
  ifneq ($(wildcard $(CODA)/$(EVIO_ARCH)/lib/libevio.*),)
    export EVIO_LIBDIR := $(CODA)/$(EVIO_ARCH)/lib
    export EVIO_INCDIR := $(CODA)/$(EVIO_ARCH)/include
    HAVE_EVIO = yes
  endif
  endif
endif
endif
ifeq ($(HAVE_EVIO),no)
# If EVIO environment not defined, define it to point here and build locally
  EVIODIR := $(shell pwd)/evio
  SUBDIRS += evio
  export EVIO_LIBDIR := $(LIBDIR)
  export EVIO_INCDIR := $(EVIODIR)
  LIBEVIO := $(LIBDIR)/libevio.$(SOSUF)
endif

HALLALIBS += -L$(EVIO_LIBDIR) -levio

ifeq ($(ARCH),linux)
# Linux with egcs (>= RedHat 5.2)
ifdef DEBUG
  CXXFLG     := -g -O0
  LDFLAGS    := -g -O0
  DEFINES    :=
else
#  CXXFLG     := -O2 -march=pentium4
  CXXFLG     := -O
  LDFLAGS    := -O
  DEFINES    := -DNDEBUG
endif
DEFINES      += -DLINUXVERS
CXXFLG       += -Wall -fPIC
CXXEXTFLG     =
LD           := $(CXX)
LDCONFIG     := /sbin/ldconfig -n $(LIBDIR)
SOFLAGS      := -shared
SONAME       := -Wl,-soname=
#FIXME: should be configure'd:
CXXVER       := $(shell g++ --version | head -1 | sed 's/.* \([0-9]\)\..*/\1/')
DEFINES      += $(shell getconf LFS_CFLAGS)
ifeq ($(CXXVER),4)
CXXEXTFLG    += -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
DICTCXXFLG   := -Wno-strict-aliasing
endif
endif

ifeq ($(ARCH),macosx)
# Mac OS X with gcc >= 3.x or clang++ >= 5
ifdef DEBUG
  CXXFLG     := -g -O0
  LDFLAGS    := -g -O0
  DEFINES    :=
else
  CXXFLG     := -O
  LDFLAGS    := -O
  DEFINES    := -DNDEBUG
endif
DEFINES      += -DMACVERS
CXXFLG       += -Wall -fPIC
CXXEXTFLG     =
LD           := $(CXX)
LDCONFIG     :=
SOFLAGS      := -shared -Wl,-undefined,dynamic_lookup
SONAME       := -Wl,-install_name,
ifeq ($(CXX),clang++)
CXXEXTFLG    += -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
else
#FIXME: should be configure'd:
CXXVER       := $(shell g++ --version | head -1 | sed 's/.* \([0-9]\)\..*/\1/')
ifeq ($(CXXVER),4)
CXXEXTFLG    += -Wextra -Wno-missing-field-initializers
DICTCXXFLG   := -Wno-strict-aliasing
endif
endif
endif

#FIXME: requires gcc 3 or up - test in configure script
DEFINES       += -DHAS_SSTREAM

# ifdef ONLINE_ET

# # ONLIBS is needed for ET
#   ET_AC_FLAGS := -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS
#   ET_CFLAGS := -02 -fPIC -I. $(ET_AC_FLAGS) -DLINUXVERS
#   LIBET  := $(CODA)/Linux/lib/libet.$(SOSUF)
#   ONLIBS := $(LIBET) -lieee -lpthread -ldl -lresolv

#   DEFINES  += -DONLINE_ET
#   HALLALIBS += $(ONLIBS)
# endif

ifdef WITH_DEBUG
DEFINES      += -DWITH_DEBUG
endif

CXXFLAGS      = $(CXXFLG) $(CXXEXTFLG) $(ROOTCFLAGS) $(INCLUDES) $(DEFINES)
CFLAGS        = $(CXXFLG) $(ROOTCFLAGS) $(INCLUDES) $(DEFINES)
LIBS         += $(ROOTLIBS) $(SYSLIBS)
GLIBS        += $(ROOTGLIBS) $(SYSLIBS)
DEFINES      += $(PODD_EXTRA_DEFINES)

export ARCH LIBDIR CXX LD SOFLAGS SONAME SOSUF CXXFLG LDFLAGS DEFINES
export VERSION SOVERSION VERCODE CXXEXTFLG

$(info Compiling for $(ARCH) with $(CXX))

#------------------------------------------------------------------------------

SRC          := src/THaFormula.cxx src/THaVform.cxx src/THaVhist.cxx \
		src/THaVar.cxx src/THaVarList.cxx src/THaCut.cxx \
		src/THaNamedList.cxx src/THaCutList.cxx src/THaInterface.cxx \
		src/THaRunBase.cxx src/THaCodaRun.cxx src/THaRun.cxx \
		src/THaRunParameters.cxx \
		src/THaDetMap.cxx src/THaApparatus.cxx src/THaDetector.cxx \
		src/THaSpectrometer.cxx src/THaSpectrometerDetector.cxx \
		src/THaHRS.cxx src/THaDecData.cxx src/BdataLoc.cxx \
		src/THaOutput.cxx src/THaString.cxx src/BankData.cxx \
		src/THaTrackingDetector.cxx src/THaNonTrackingDetector.cxx \
		src/THaPidDetector.cxx src/THaSubDetector.cxx \
		src/THaAnalysisObject.cxx src/THaDetectorBase.cxx \
		src/THaRTTI.cxx src/THaPhysicsModule.cxx \
		src/THaVertexModule.cxx src/THaTrackingModule.cxx \
		src/THaAnalyzer.cxx src/THaPrintOption.cxx \
		src/THaBeam.cxx src/THaIdealBeam.cxx \
		src/THaRasteredBeam.cxx src/THaRaster.cxx\
		src/THaBeamDet.cxx src/THaBPM.cxx src/THaUnRasteredBeam.cxx\
		src/THaTrack.cxx src/THaPIDinfo.cxx src/THaParticleInfo.cxx \
		src/THaCluster.cxx src/THaArrayString.cxx \
		src/THaScintillator.cxx src/THaShower.cxx \
		src/THaTotalShower.cxx src/THaCherenkov.cxx \
		src/THaEvent.cxx src/THaTrackID.cxx src/THaVDC.cxx \
		src/THaVDCPlane.cxx src/THaVDCChamber.cxx src/THaVDCPoint.cxx \
		src/THaVDCWire.cxx src/THaVDCHit.cxx src/THaVDCCluster.cxx \
		src/THaVDCTimeToDistConv.cxx src/THaVDCTrackID.cxx \
                src/THaVDCAnalyticTTDConv.cxx \
		src/THaVDCPointPair.cxx src/VDCeff.cxx \
		src/THaElectronKine.cxx src/THaReactionPoint.cxx \
		src/THaReacPointFoil.cxx \
		src/THaTwoarmVertex.cxx src/THaAvgVertex.cxx \
		src/THaExtTarCor.cxx src/THaDebugModule.cxx \
		src/THaTrackInfo.cxx src/THaGoldenTrack.cxx \
		src/THaPrimaryKine.cxx src/THaSecondaryKine.cxx \
	        src/THaCoincTime.cxx src/THaS2CoincTime.cxx \
                src/THaTrackProj.cxx src/THaPostProcess.cxx src/THaFilter.cxx \
		src/THaElossCorrection.cxx src/THaTrackEloss.cxx \
		src/THaBeamModule.cxx src/THaBeamInfo.cxx \
		src/THaEpicsEbeam.cxx src/THaBeamEloss.cxx \
		src/THaTrackOut.cxx src/THaTriggerTime.cxx \
		src/THaHelicityDet.cxx src/THaG0HelicityReader.cxx \
		src/THaG0Helicity.cxx src/THaADCHelicity.cxx \
		src/THaHelicity.cxx src/THaPhotoReaction.cxx \
		src/THaSAProtonEP.cxx \
		src/THaTextvars.cxx src/THaQWEAKHelicity.cxx \
		src/THaQWEAKHelicityReader.cxx src/THaEvtTypeHandler.cxx \
		src/THaScalerEvtHandler.cxx src/THaEpicsEvtHandler.cxx \
		src/THaEvt125Handler.cxx src/FileInclude.cxx \
		src/Variable.cxx src/VariableArrayVar.cxx \
		src/FixedArrayVar.cxx src/VectorVar.cxx src/MethodVar.cxx \
		src/SeqCollectionVar.cxx src/SeqCollectionMethodVar.cxx \
		src/VectorObjVar.cxx src/VectorObjMethodVar.cxx

# ifdef ONLINE_ET
# SRC += src/THaOnlRun.cxx
# endif

OBJ          := $(SRC:.cxx=.o)
RCHDR        := $(SRC:.cxx=.h) src/THaGlobals.h
HDR          := $(RCHDR) src/VarDef.h src/VarType.h src/ha_compiledata.h
DEP          := $(SRC:.cxx=.d) src/main.d
OBJS         := $(OBJ) $(HA_DICT).o
HA_LINKDEF   := src/HallA_LinkDef.h

LIBHALLA     := $(LIBDIR)/libHallA.$(SOSUF)
LIBDC        := $(LIBDIR)/libdc.$(SOSUF)

#------------------------------------------------

PROGRAMS     := analyzer
PODDLIBS     := $(LIBHALLA) $(LIBDC)
ifdef EVIODIR
PODDLIBS     += $(LIBEVIO)
endif

all:            subdirs
		set -e; for i in $(PROGRAMS); do $(MAKE) $$i; done

src/ha_compiledata.h:	Makefile
		@echo "#ifndef ANALYZER_COMPILEDATA_H" > $@
		@echo "#define ANALYZER_COMPILEDATA_H" >> $@
		@echo "" >> $@
		@echo "#define HA_INCLUDEPATH \"$(INCDIRS)\"" >> $@
		@echo "#define HA_VERSION \"$(VERSION)$(EXTVERS)\"" >> $@
		@echo "#define HA_DATE \"$(shell date '+%b %d %Y')\"" >> $@
#		@echo "#define HA_DATETIME \"$(shell date '+%a %b %d %H:%M:%S %Z %Y')\"" >> $@
		@echo "#define HA_DATETIME \"$(shell date '+%a %b %d %Y')\"" >> $@
		@echo "#define HA_PLATFORM \"$(shell uname -s)-$(shell uname -r)-$(shell uname -m)\"" >> $@
		@echo "#define HA_BUILDNODE \"$(shell uname -n)\"" >> $@
		@echo "#define HA_BUILDDIR \"$(shell pwd)\"" >> $@
		@echo "#define HA_BUILDUSER \"$(shell whoami)\"" >> $@
		@echo "#define HA_GITVERS \"$(shell git rev-parse HEAD 2>/dev/null | cut -c1-7)\"" >> $@
		@echo "#define HA_CXXVERS \"$(shell $(CXX) --version 2>/dev/null | head -1)\"" >> $@
		@echo "#define HA_ROOTVERS \"$(shell root-config --version)\"" >> $@
		@echo "#define ANALYZER_VERSION_CODE $(VERCODE)" >> $@
		@echo "#define ANALYZER_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))" >> $@
		@echo "" >> $@
		@echo "#endif" >> $@

src/THaInterface.o:  src/ha_compiledata.h

subdirs:	$(SUBDIRS)

$(SUBDIRS):
		set -e; $(MAKE) -C $@

#---------- Core libraries -----------------------------------------
$(LIBHALLA).$(VERSION):	$(HDR) $(OBJS)
ifeq ($(strip $(SONAME)),)
		$(LD) $(LDFLAGS) $(SOFLAGS) -o $@ $(OBJS)
else
		$(LD) $(LDFLAGS) $(SOFLAGS) $(SONAME)$(notdir $(LIBHALLA).$(SOVERSION)) -o $@ $(OBJS)
endif
		@echo "$@ done"

$(LIBHALLA).$(SOVERSION):	$(LIBHALLA).$(VERSION)
ifneq ($(strip $(LDCONFIG)),)
		$(LDCONFIG)
else
		rm -f $@
		ln -s $(notdir $<) $@
endif

$(LIBHALLA):	$(LIBHALLA).$(SOVERSION)
		cd $(LIBDIR)
		rm -f $@
		ln -s $(notdir $<) $@

$(LIBDC).$(SOVERSION):	$(LIBDC).$(VERSION)
ifneq ($(strip $(LDCONFIG)),)
		$(LDCONFIG)
else
		rm -f $@
		ln -s $(notdir $<) $@
endif

ifneq ($(strip $(LDCONFIG)),)
		$(LDCONFIG)
else
		rm -f $@
		ln -s $(notdir $<) $@
endif

ifdef EVIODIR
$(DCDIR):	evio
endif

$(LIBDC):	$(LIBDC).$(SOVERSION)
		rm -f $@
		ln -s $(notdir $<) $@

ifeq ($(ARCH),linux)
$(HA_DICT).o:  $(HA_DICT).cxx
	$(CXX) $(CXXFLAGS) $(DICTCXXFLG) -o $@ -c $^
endif

$(HA_DICT).cxx: $(RCHDR) $(HA_LINKDEF)
	@echo "Generating dictionary $(HA_DICT)..."
	$(ROOTBIN)/rootcint -f $@ -c $(ROOTINC) $(INCLUDES) $(DEFINES) $^


#---------- Main program -------------------------------------------
analyzer:	src/main.o $(PODDLIBS)
		$(LD) $(LDFLAGS) $< $(HALLALIBS) $(GLIBS) -o $@

#---------- Maintenance --------------------------------------------
clean:
		set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done
		rm -f *.$(SOSUF) *.{a,o,os} *.$(SOSUF).* site_scons/*.pyc *~
		rm -f $(PROGRAMS) $(HA_DICT).* $(HA_DICT)_rdict.pcm THaDecDict_rdict.pcm
		cd src; rm -f ha_compiledata.h *.{o,os} *~

realclean:	clean
		set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i realclean; done
		find . -name "*.d" -exec rm {} \;

srcdist:
		rm -f ../$(NAME)
		ln -s $(PWD) ../$(NAME)
		tar -czv -f ../$(NAME).tar.gz -X .exclude -C .. \
		 $(addprefix $(NAME)/, \
		  ChangeLog $(wildcard README*) Makefile .exclude .gitignore \
		  $(wildcard SCons*) site_scons \
		  src $(DCDIR) Calib DB examples contrib utils docs SDK \
		  evio/Makefile evio/Makefile.libsrc)

install:	all
ifndef ANALYZER
		$(error $$ANALYZER environment variable not defined)
endif
ifneq ($(ANALYZER),$(shell pwd))
		@echo "Installing in $(ANALYZER) ..."
		@mkdir -p $(ANALYZER)/{$(PLATFORM),include,src/src,docs,DB,examples,SDK}
		cp -pu $(SRC) $(HDR) $(HA_LINKDEF) $(ANALYZER)/src/src
		cp -pu $(HDR) $(ANALYZER)/include
		tar cf - $(shell find examples docs SDK -type f | grep -v '*~') | \
			tar xf - -C $(ANALYZER)
		cp -pu Makefile ChangeLog $(ANALYZER)/src
		cp -pru DB $(ANALYZER)/
		@echo "Installing in $(ANALYZER)/$(PLATFORM) ..."
		for lib in $(filter-out $(LIBEVIO), $(PODDLIBS)); do \
			rm -f  $(ANALYZER)/$(PLATFORM)/$(notdir $$lib); \
			rm -f  $(ANALYZER)/$(PLATFORM)/$(notdir $$lib).$(SOVERSION); \
			rm -f  $(ANALYZER)/$(PLATFORM)/$(notdir $$lib).$(VERSION); \
			cp -af $$lib $$lib.$(SOVERSION) $$lib.$(VERSION) \
			   $(ANALYZER)/$(PLATFORM) 2>/dev/null; \
		done
		rm -f $(ANALYZER)/$(PLATFORM)/libevio.$(SOSUF)
ifdef LIBEVIO
		cp -af $(LIBEVIO) $(ANALYZER)/$(PLATFORM)
endif
		rm -f $(ANALYZER)/$(PLATFORM)/analyzer $(ANALYZER)/$(PLATFORM)/$(NAME)
		cp -pf $(PROGRAMS) $(ANALYZER)/$(PLATFORM)/
ifneq ($(PLATFORM),bin)
		rm -f $(ANALYZER)/bin
		ln -s $(ANALYZER)/$(PLATFORM) $(ANALYZER)/bin
endif
ifneq ($(NAME),analyzer)
		mv $(ANALYZER)/$(PLATFORM)/analyzer $(ANALYZER)/$(PLATFORM)/$(NAME)
		ln -s $(NAME) $(ANALYZER)/$(PLATFORM)/analyzer
endif
		set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i install; done
else
		@echo "Everything already installed"
endif

###--- DO NOT CHANGE ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT
###    YOU ARE DOING

.PHONY: all clean realclean srcdist subdirs $(SUBDIRS)

.SUFFIXES:
.SUFFIXES: .c .cc .cpp .cxx .o .os .d

%.o:	%.cxx
ifeq ($(strip $(MAKEDEPEND)),)
	$(CXX) $(CXXFLAGS) -MMD -o $@ -c $<
	@mv -f $*.d $*.d.tmp
else
	$(CXX) $(CXXFLAGS) -o $@ -c $<
	$(MAKEDEPEND) $(ROOTINC) $(INCLUDES) $(DEFINES) -c $< > $*.d.tmp
endif
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

-include $(DEP)

