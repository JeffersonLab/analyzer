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
SOVERSION  := 1.5
PATCH   := 36
VERSION := $(SOVERSION).$(PATCH)
EXTVERS :=
#EXTVERS := -et
NAME    := analyzer-$(VERSION)
VERCODE := $(shell echo $(subst ., ,$(SOVERSION)) $(PATCH) | \
   awk '{ print $$1*65536 + $$2*256 + $$3 }')

#------------------------------------------------------------------------------

MACHINE := $(shell uname -s)
ARCH    := linux
ifeq ($(MACHINE),Darwin)
  ARCH := macosx
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
SCALERDIR    := hana_scaler
LIBDIR       := $(shell pwd)
HALLALIBS    := -L$(LIBDIR) -lHallA -ldc -lscaler
SUBDIRS      := $(DCDIR) $(SCALERDIR)
INCDIRS      := $(addprefix $(HA_DIR)/, src $(SUBDIRS))
HA_DICT      := haDict

LIBS         :=
GLIBS        :=

INCLUDES     := $(addprefix -I, $(INCDIRS) )

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
CXXEXTFLG     = -Woverloaded-virtual
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
# Mac OS X with Xcode/gcc 3.x
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
CXXEXTFLG     = -Woverloaded-virtual
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
CXXEXTFLG    += -Wextra -Wno-missing-field-initializers -Wno-unused-parameter
DICTCXXFLG   := -Wno-strict-aliasing
endif
endif
endif

#FIXME: requires gcc 3 or up - test in configure script
DEFINES       += -DHAS_SSTREAM

ifdef ONLINE_ET

# ONLIBS is needed for ET
  ET_AC_FLAGS := -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS
  ET_CFLAGS := -02 -fPIC -I. $(ET_AC_FLAGS) -DLINUXVERS
# CODA environment variable must be set.  Examples are
#   CODA:= /adaqfs/coda/2.2        (in adaq cluster)
#   CODA:= /data7/user/coda/2.2    (on haplix cluster)
  LIBET  := $(CODA)/Linux/lib/libet.so
  ONLIBS := $(LIBET) -lieee -lpthread -ldl -lresolv

  DEFINES  += -DONLINE_ET
  HALLALIBS += $(ONLIBS)
endif


ifdef WITH_DEBUG
DEFINES      += -DWITH_DEBUG
endif

CXXFLAGS      = $(CXXFLG) $(CXXEXTFLG) $(ROOTCFLAGS) $(INCLUDES) $(DEFINES)
CFLAGS        = $(CXXFLG) $(ROOTCFLAGS) $(INCLUDES) $(DEFINES)
LIBS         += $(ROOTLIBS) $(SYSLIBS)
GLIBS        += $(ROOTGLIBS) $(SYSLIBS)
DEFINES      += $(PODD_EXTRA_DEFINES)

export ARCH LIBDIR CXX LD SOFLAGS SONAME CXXFLG LDFLAGS DEFINES VERSION SOVERSION VERCODE CXXEXTFLG

$(info Compiling for $(ARCH) with $(CXX))

#------------------------------------------------------------------------------

SRC          := src/THaFormula.C src/THaVform.C src/THaVhist.C \
		src/THaVar.C src/THaVarList.C src/THaCut.C \
		src/THaNamedList.C src/THaCutList.C src/THaInterface.C \
		src/THaRunBase.C src/THaCodaRun.C src/THaRun.C \
		src/THaRunParameters.C \
		src/THaDetMap.C src/THaApparatus.C src/THaDetector.C \
		src/THaSpectrometer.C src/THaSpectrometerDetector.C \
		src/THaHRS.C \
                src/THaDecData.C src/THaOutput.C src/THaString.C \
		src/THaTrackingDetector.C src/THaNonTrackingDetector.C \
		src/THaPidDetector.C src/THaSubDetector.C \
		src/THaAnalysisObject.C src/THaDetectorBase.C src/THaRTTI.C \
		src/THaPhysicsModule.C src/THaVertexModule.C \
		src/THaTrackingModule.C \
		src/THaAnalyzer.C src/THaPrintOption.C \
		src/THaBeam.C src/THaIdealBeam.C \
		src/THaRasteredBeam.C src/THaRaster.C\
		src/THaBeamDet.C src/THaBPM.C src/THaUnRasteredBeam.C\
		src/THaTrack.C src/THaPIDinfo.C src/THaParticleInfo.C \
		src/THaCluster.C src/THaArrayString.C \
		src/THaScintillator.C src/THaShower.C \
		src/THaTotalShower.C src/THaCherenkov.C \
		src/THaEvent.C src/THaTrackID.C src/THaVDC.C \
		src/THaVDCPlane.C src/THaVDCUVPlane.C src/THaVDCUVTrack.C \
		src/THaVDCWire.C src/THaVDCHit.C src/THaVDCCluster.C \
		src/THaVDCTimeToDistConv.C src/THaVDCTrackID.C \
                src/THaVDCAnalyticTTDConv.C \
		src/THaVDCTrackPair.C src/THaScalerGroup.C \
		src/THaElectronKine.C src/THaReactionPoint.C \
		src/THaReacPointFoil.C \
		src/THaTwoarmVertex.C src/THaAvgVertex.C \
		src/THaExtTarCor.C src/THaDebugModule.C src/THaTrackInfo.C \
		src/THaGoldenTrack.C \
		src/THaPrimaryKine.C src/THaSecondaryKine.C \
	        src/THaCoincTime.C src/THaS2CoincTime.C \
                src/THaTrackProj.C \
		src/THaPostProcess.C src/THaFilter.C \
		src/THaElossCorrection.C src/THaTrackEloss.C \
		src/THaBeamModule.C src/THaBeamInfo.C src/THaEpicsEbeam.C \
		src/THaBeamEloss.C \
		src/THaTrackOut.C src/THaTriggerTime.C \
		src/THaHelicityDet.C src/THaG0HelicityReader.C \
		src/THaG0Helicity.C src/THaADCHelicity.C src/THaHelicity.C \
		src/THaPhotoReaction.C src/THaSAProtonEP.C \
		src/THaTextvars.C src/THaQWEAKHelicity.C \
		src/THaQWEAKHelicityReader.C

ifdef ONLINE_ET
SRC += src/THaOnlRun.C
endif

OBJ          := $(SRC:.C=.o)
RCHDR        := $(SRC:.C=.h) src/THaGlobals.h
HDR          := $(RCHDR) src/VarDef.h src/VarType.h src/ha_compiledata.h
DEP          := $(SRC:.C=.d) src/main.d
OBJS         := $(OBJ) $(HA_DICT).o
HA_LINKDEF   := src/HallA_LinkDef.h

LIBHALLA     := $(LIBDIR)/libHallA.so
LIBDC        := $(LIBDIR)/libdc.so
LIBSCALER    := $(LIBDIR)/libscaler.so

#------ Extra libraries -------------------------
LNA          := NormAna
LIBNORMANA   := $(LIBDIR)/lib$(LNA).so
LNA_DICT     := $(LNA)Dict
LNA_SRC      := src/THa$(LNA).C
LNA_OBJ      := $(LNA_SRC:.C=.o)
LNA_HDR      := $(LNA_SRC:.C=.h)
LNA_DEP      := $(LNA_SRC:.C=.d)
LNA_OBJS     := $(LNA_OBJ) $(LNA_DICT).o
LNA_LINKDEF  := src/$(LNA)_LinkDef.h
#------------------------------------------------

PROGRAMS     := analyzer $(LIBNORMANA)
PODDLIBS     := $(LIBHALLA) $(LIBDC) $(LIBSCALER)

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

$(LIBSCALER).$(SOVERSION):	$(LIBSCALER).$(VERSION)
ifneq ($(strip $(LDCONFIG)),)
		$(LDCONFIG)
else
		rm -f $@
		ln -s $(notdir $<) $@
endif

$(LIBDC):	$(LIBDC).$(SOVERSION)
		rm -f $@
		ln -s $(notdir $<) $@

$(LIBSCALER):	$(LIBSCALER).$(SOVERSION)
		rm -f $@
		ln -s $(notdir $<) $@

ifeq ($(ARCH),linux)
$(HA_DICT).o:  $(HA_DICT).C
	$(CXX) $(CXXFLAGS) $(DICTCXXFLG) -o $@ -c $^
endif

$(HA_DICT).C: $(RCHDR) $(HA_LINKDEF)
	@echo "Generating dictionary $(HA_DICT)..."
	$(ROOTBIN)/rootcint -f $@ -c $(ROOTINC) $(INCLUDES) $(DEFINES) $^


#---------- Extra libraries ----------------------------------------

$(LIBNORMANA):	$(LNA_HDR) $(LNA_OBJS)
		$(LD) $(LDFLAGS) $(SOFLAGS) -o $@ $(LNA_OBJS)
		@echo "$@ done"

$(LNA_DICT).C:	$(LNA_HDR) $(LNA_LINKDEF)
		@echo "Generating dictionary $(LNA_DICT)..."
		rootcint -f $@ -c $(ROOTINC) $(INCLUDES) $(DEFINES) $^

libPodd.a:	$(OBJS) $(LNA_OBJS) subdirs
		echo "Making libPodd.a..."
		ar rcs $@ $(OBJS) $(LNA_OBJS) $(DCDIR)/*.o $(SCALERDIR)/*.o

#---------- Main program -------------------------------------------
analyzer:	src/main.o $(PODDLIBS)
		$(LD) $(LDFLAGS) $< $(HALLALIBS) $(GLIBS) -o $@

#---------- Maintenance --------------------------------------------
clean:
		set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done
		rm -f *.{so,a,o,os} *.so.* *~ *.pyc
		rm -f $(PROGRAMS) $(HA_DICT).* $(LNA_DICT).* *_rdict.pcm
		cd src; rm -f ha_compiledata.h *.{o,os} *~

realclean:	clean
		set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i realclean; done
		find . -name "*.d" -exec rm {} \;

srcdist:
		rm -f ../$(NAME)
		ln -s $(PWD) ../$(NAME)
		tar -czvf ../$(NAME).tar.gz -X .exclude -C .. \
		$(addprefix $(NAME)/, \
                  ChangeLog Makefile .exclude $(wildcard SCons*) site_scons \
                  src $(DCDIR) $(SCALERDIR) Calib DB examples contrib docs SDK)

install:	all
ifndef ANALYZER
		$(error $$ANALYZER environment variable not defined)
endif
		@echo "Installing in $(ANALYZER) ..."
		@mkdir -p $(ANALYZER)/{$(PLATFORM),include,src/src,docs,DB,examples,SDK}
		cp -pu $(SRC) $(HDR) $(HA_LINKDEF) $(ANALYZER)/src/src
		cp -pu $(LNA_SRC) $(LNA_HDR) $(LNA_LINKDEF) $(ANALYZER)/src/src
		cp -pu $(HDR) $(LNA_HDR) $(ANALYZER)/include
		tar cf - `find examples docs SDK -type f | grep -Ev '(CVS|*~)'` | tar xf - -C $(ANALYZER)
		cp -pu Makefile ChangeLog $(ANALYZER)/src
		cp -pru DB $(ANALYZER)/
		@echo "Installing in $(ANALYZER)/$(PLATFORM) ..."
		for lib in $(PODDLIBS); do \
			rm -f  $(ANALYZER)/$(PLATFORM)/$(notdir $$lib); \
			rm -f  $(ANALYZER)/$(PLATFORM)/$(notdir $$lib).$(SOVERSION); \
			rm -f  $(ANALYZER)/$(PLATFORM)/$(notdir $$lib).$(VERSION); \
			cp -af $$lib $$lib.$(SOVERSION) $$lib.$(VERSION) $(ANALYZER)/$(PLATFORM); \
		done
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

###--- DO NOT CHANGE ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT
###    YOU ARE DOING

.PHONY: all clean realclean srcdist cvsdist subdirs $(SUBDIRS)

.SUFFIXES:

%.o:	%.C
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

