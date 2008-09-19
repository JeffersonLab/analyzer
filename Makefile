#------------------------------------------------------------------------------

# Compile extra debugging code (slight performance impact)
export WITH_DEBUG = 1

# Compile debug version
#export DEBUG = 1

# Profiling with gprof
# export PROFILE = 1

# Include libraries for reading from the ET ring
#  (only for adaq? machines with the Coda libraries )
# export ONLINE_ET = 1
#------------------------------------------------------------------------------

# SOVERSION should be numerical only - it becomes the shared lib soversion
# EXTVERS (optional) describes the build, e.g. "dbg", "et", "gcc33" etc.
SOVERSION  = 1.4
PATCH   = 12
VERSION = $(SOVERSION).$(PATCH)
EXTVERS =
NAME    = analyzer-$(VERSION)

#------------------------------------------------------------------------------

ARCH          = linuxegcs
#ARCH          = solarisCC5
ifndef PLATFORM
PLATFORM = bin
endif

ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLIBS     := $(shell root-config --libs)
ROOTGLIBS    := $(shell root-config --glibs)

HA_DIR       := $(shell pwd)
DCDIR         = hana_decode
SCALERDIR     = hana_scaler
LIBDIR        = $(shell pwd)
HALLALIBS     = -L$(LIBDIR) -lHallA -ldc -lscaler
SUBDIRS       = $(DCDIR) $(SCALERDIR)
INCDIRS       = $(addprefix $(HA_DIR)/, src $(SUBDIRS))

LIBS          = 
GLIBS         = 

INCLUDES      = $(ROOTCFLAGS) $(addprefix -I, $(INCDIRS) )

ifeq ($(ARCH),solarisCC5)
# Solaris CC 5.0
CXX           = CC
ifdef DEBUG
  CXXFLG      = -g
  LDFLAGS     = -g
else
  CXXFLG      = -O
  LDFLAGS     = -O
endif
CXXFLG       += -KPIC
LD            = CC
LDCONFIG      =
SOFLAGS       = -G
SONAME        = -h
DEFINES       = -DSUNVERS
endif

ifeq ($(ARCH),linuxegcs)
# Linux with egcs (>= RedHat 5.2)
CXX           = g++
ifdef DEBUG
  CXXFLG      = -g -O0
  LDFLAGS     = -g -O0
else
  CXXFLG      = -O
#  CXXFLG      = -O -march=pentium4 #-msse2 -mfpmath=sse
  LDFLAGS     = -O
endif
DEFINES       = -DLINUXVERS
CXXFLG       += -Wall -fPIC
CXXEXTFLG     = -Woverloaded-virtual
LD            = g++
LDCONFIG      = /sbin/ldconfig -n $(LIBDIR)
SOFLAGS       = -shared
SONAME        = -Wl,-soname=

#FIXME: requires gcc 3 or up - test in configure script
DEFINES       += -DHAS_SSTREAM

endif

ifeq ($(CXX),)
$(error $(ARCH) invalid architecture)
endif

ifdef ONLINE_ET

# ONLIBS is needed for ET
  ET_AC_FLAGS = -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS
  ET_CFLAGS = -02 -fPIC -I. $(ET_AC_FLAGS) -DLINUXVERS
# CODA environment variable must be set.  Examples are
#   CODA = /adaqfs/coda/2.2        (in adaq cluster)
#   CODA = /data7/user/coda/2.2    (on haplix cluster)
  LIBET = $(CODA)/Linux/lib/libet.so
  ONLIBS = $(LIBET) -lieee -lpthread -ldl -lresolv

  DEFINES  += -DONLINE_ET
  HALLALIBS += $(ONLIBS)
endif


MAKEDEPEND    = gcc

ifdef WITH_DEBUG
DEFINES      += -DWITH_DEBUG
endif

ifdef PROFILE
CXXFLG       += -pg
LDFLAGS      += -pg
endif

CXXFLAGS      = $(CXXFLG) $(CXXEXTFLG) $(INCLUDES) $(DEFINES)
CFLAGS        = $(CXXFLG) $(INCLUDES) $(DEFINES)
LIBS         += $(ROOTLIBS) $(SYSLIBS)
GLIBS        += $(ROOTGLIBS) $(SYSLIBS)

export ARCH LIBDIR CXX LD SOFLAGS SONAME CXXFLG LDFLAGS DEFINES VERSION SOVERSION CXXEXTFLG

#------------------------------------------------------------------------------

SRC           = src/THaFormula.C src/THaVform.C src/THaVhist.C \
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
		src/THaEvent.C src/THaRawEvent.C src/THaTrackID.C \
		src/THaVDC.C src/THaVDCEvent.C \
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
		src/THaDB.C src/THaDBFile.C \
	        src/THaCoincTime.C src/THaS2CoincTime.C \
                src/THaTrackProj.C \
		src/THaPostProcess.C src/THaFilter.C \
		src/THaElossCorrection.C src/THaTrackEloss.C \
		src/THaBeamModule.C src/THaBeamInfo.C src/THaEpicsEbeam.C \
		src/THaBeamEloss.C \
		src/THaTrackOut.C src/THaTriggerTime.C \
		src/THaPhotoReaction.C src/THaSAProtonEP.C

ifdef ONLINE_ET
SRC += src/THaOnlRun.C
endif

OBJ           = $(SRC:.C=.o)
RCHDR         = $(SRC:.C=.h) src/THaGlobals.h
HDR           = $(RCHDR) src/VarDef.h src/VarType.h src/ha_compiledata.h src/Ext_TRotation.h
DEP           = $(SRC:.C=.d) src/main.d
OBJS          = $(OBJ) haDict.o
HA_LINKDEF    = src/HallA_LinkDef.h

LIBHALLA      = $(LIBDIR)/libHallA.so
LIBDC         = $(LIBDIR)/libdc.so
LIBSCALER     = $(LIBDIR)/libscaler.so

#------ Extra libraries -------------------------
LNA           = NormAna
LIBNORMANA    = $(LIBDIR)/lib$(LNA).so
LNA_DICT      = $(LNA)Dict
LNA_SRC       = src/THa$(LNA).C
LNA_OBJ       = $(LNA_SRC:.C=.o)
LNA_HDR       = $(LNA_SRC:.C=.h)
LNA_DEP       = $(LNA_SRC:.C=.d)
LNA_OBJS      = $(LNA_OBJ) $(LNA_DICT).o
LNA_LINKDEF   = src/$(LNA)_LinkDef.h
#------------------------------------------------

PROGRAMS      = analyzer $(LIBNORMANA)

all:            subdirs
		set -e; for i in $(PROGRAMS); do $(MAKE) $$i; done

src/ha_compiledata.h:	Makefile
		echo "#define HA_INCLUDEPATH \"$(INCDIRS)\"" > $@
		echo "#define HA_VERSION \"$(VERSION)$(EXTVERS)\"" >> $@

subdirs:
		set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i; done

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
		ln -s $< $@
endif

$(LIBHALLA):	$(LIBHALLA).$(SOVERSION)
		rm -f $@
		ln -s $< $@

$(LIBDC).$(SOVERSION):	$(LIBDC).$(VERSION)
ifneq ($(strip $(LDCONFIG)),)
		$(LDCONFIG)
else
		rm -f $@
		ln -s $< $@
endif

$(LIBSCALER).$(SOVERSION):	$(LIBSCALER).$(VERSION)
ifneq ($(strip $(LDCONFIG)),)
		$(LDCONFIG)
else
		rm -f $@
		ln -s $< $@
endif

$(LIBDC):	$(LIBDC).$(SOVERSION)
		rm -f $@
		ln -s $< $@

$(LIBSCALER):	$(LIBSCALER).$(SOVERSION)
		rm -f $@
		ln -s $< $@

haDict.C: $(RCHDR) $(HA_LINKDEF)
	@echo "Generating dictionary haDict..."
	$(ROOTSYS)/bin/rootcint -f $@ -c $(INCLUDES) $(DEFINES) $^


#---------- Extra libraries ----------------------------------------

$(LIBNORMANA):	$(LNA_HDR) $(LNA_OBJS)
		$(LD) $(LDFLAGS) $(SOFLAGS) -o $@ $(LNA_OBJS)
		@echo "$@ done"

$(LNA_DICT).C:	$(LNA_HDR) $(LNA_LINKDEF)
		@echo "Generating dictionary $(LNA_DICT)..."
		$(ROOTSYS)/bin/rootcint -f $@ -c $(INCLUDES) $(DEFINES) $^

#---------- Main program -------------------------------------------
analyzer:	src/main.o $(LIBDC) $(LIBSCALER) $(LIBHALLA)
		$(LD) $(LDFLAGS) $< $(HALLALIBS) $(GLIBS) -o $@


#---------- Maintenance --------------------------------------------
clean:
		set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done
		rm -f *.so *.a $(PROGRAMS) *.o *Dict.* *~ src/ha_compiledata.h
		cd src; rm -f *.o *~

realclean:	clean
		set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i realclean; done
		find . -name "*.d" -exec rm {} \;

srcdist:
		rm -f ../$(NAME)
		ln -s $(PWD) ../$(NAME)
		gtar czv -C .. -f ../$(NAME).tar.gz -X .exclude \
		 -V "JLab/Hall A C++ Analysis Software "$(VERSION)" `date -I`"\
		 $(NAME)/.exclude $(NAME)/ChangeLog \
		 $(NAME)/src $(NAME)/examples \
		 $(NAME)/DB $(NAME)/$(DCDIR) $(NAME)/$(SCALERDIR) \
		 $(NAME)/Makefile $(NAME)/docs $(NAME)/Calib $(NAME)/contrib

cvsdist:	srcdist
		cp ../$(NAME).tar.gz ../$(NAME)-cvs.tar.gz
		gunzip -f ../$(NAME)-cvs.tar.gz
		gtar rv -C .. -f ../$(NAME)-cvs.tar \
		 `find . -type d -name CVS 2>/dev/null | sed "s%^\.%$(NAME)%"`\
		 `find . -type f -name .cvsignore 2>/dev/null | sed "s%^\./%$(NAME)/%"`
		gzip -f ../$(NAME)-cvs.tar

install:	all
ifndef ANALYZER
		$(error $$ANALYZER environment variable not defined)
endif
		@echo "Installing in $(ANALYZER) ..."
		@mkdir -p $(ANALYZER)/{$(PLATFORM),include,src/src,docs,DB,examples,SDK}
		cp -pu $(SRC) $(HDR) $(HA_LINKDEF) $(ANALYZER)/src/src
		cp -pu $(LNA_SRC) $(LNA_HDR) $(LNA_LINKDEF) $(ANALYZER)/src/src
		cp -pu $(HDR) $(LNA_HDR) $(ANALYZER)/include
		gtar cf - `find examples docs SDK -type f | grep -Ev '(CVS|*~)'` | gtar xf - -C $(ANALYZER)
		cp -pu Makefile ChangeLog $(ANALYZER)/src
		cp -pru DB $(ANALYZER)/
		@echo "Installing in $(ANALYZER)/$(PLATFORM) ..."
		rm -f $(ANALYZER)/$(PLATFORM)/lib*.so.$(SOVERSION)
		rm -f $(ANALYZER)/$(PLATFORM)/lib*.so.$(SOVERSION).*
		rm -f $(ANALYZER)/$(PLATFORM)/analyzer
		rm -f $(ANALYZER)/$(PLATFORM)/analyzer-$(SOVERSION).*
		cp -af lib*.so.$(SOVERSION) lib*.so.$(VERSION) $(ANALYZER)/$(PLATFORM)
		cp -pf $(PROGRAMS) $(ANALYZER)/$(PLATFORM)/
		mv $(ANALYZER)/$(PLATFORM)/analyzer $(ANALYZER)/$(PLATFORM)/$(NAME)
		ln -s $(NAME) $(ANALYZER)/$(PLATFORM)/analyzer
		set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i install; done

.PHONY: all clean realclean srcdist cvsdist subdirs


###--- DO NOT CHANGE ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT 
###    YOU ARE DOING

.SUFFIXES:
.SUFFIXES: .c .cc .cpp .C .o .d

%.o:	%.C
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.d:	%.C src/ha_compiledata.h
	@echo Creating dependencies for $<
#	@$(SHELL) -ec '$(CXX) -MM $(CXXFLAGS) -c $< \
#		| sed '\''s%\($*\)\.o[ :]*%\1.o $@ : %g'\'' > $@; \
#		[ -s $@ ] || rm -f $@'
	@$(SHELL) -ec '$(MAKEDEPEND) -MM $(INCLUDES) $(DEFINES) -c $< \
		| sed '\''s%^.*\.o%$*\.o%g'\'' \
		| sed '\''s%\($*\)\.o[ :]*%\1.o $@ : %g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

###

-include $(DEP)

