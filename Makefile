#------------------------------------------------------------------------------

# Compile extra debugging code (slight performance impact)
export WITH_DEBUG = 1

# Compile debug version
export DEBUG = 1

# Profiling with gprof
# export PROFILE = 1

#------------------------------------------------------------------------------

VERSION = 0.66

#------------------------------------------------------------------------------

ARCH          = linuxegcs
#ARCH          = solarisCC5

ROOTCFLAGS   := $(shell root-config --cflags)
ROOTLIBS     := $(shell root-config --libs)
ROOTGLIBS    := $(shell root-config --glibs)

DCDIR         = hana_decode
DCLIB         = -L. -ldc
SCALERDIR     = hana_scaler
SCALERLIB     = -L. -lscaler
HALLALIBS     = -L. -lHallA -ldc -lscaler

LIBS          = 
GLIBS         = 

INCLUDES      = $(ROOTCFLAGS) -I$(DCDIR) -I$(SCALERDIR)

ifeq ($(ARCH),solarisCC5)
# Solaris CC 5.0
CXX           = CC
ifdef DEBUG
  CXXFLAGS    = -g
  LDFLAGS     = -g
else
  CXXFLAGS    = -O
  LDFLAGS     = -O
endif
CXXFLAGS     += -KPIC
LD            = CC
SOFLAGS       = -G
endif

ifeq ($(ARCH),linuxegcs)
# Linux with egcs (>= RedHat 5.2)
CXX           = g++
ifdef DEBUG
  CXXFLAGS    = -g -O0
  LDFLAGS     = -g -O0
else
  CXXFLAGS    = -O
  LDFLAGS     = -O
endif
CXXFLAGS     += -Wall -Woverloaded-virtual -fPIC
LD            = g++
SOFLAGS       = -shared
endif

ifeq ($(CXX),)
$(error $(ARCH) invalid architecture)
endif

CXXFLAGS     += $(INCLUDES)
LIBS         += $(ROOTLIBS) $(SYSLIBS)
GLIBS        += $(ROOTGLIBS) $(SYSLIBS)

MAKEDEPEND    = gcc

ifdef WITH_DEBUG
CXXFLAGS     += -DWITH_DEBUG
endif

ifdef PROFILE
CXXFLAGS     += -pg
LDFLAGS      += -pg
endif

export ARCH

#------------------------------------------------------------------------------

SRC           = src/THaFormula.C src/THaVar.C src/THaVarList.C src/THaCut.C \
		src/THaNamedList.C src/THaCutList.C src/THaInterface.C \
		src/THaRun.C \
		src/THaDetMap.C src/THaApparatus.C src/THaDetector.C \
		src/THaSpectrometer.C src/THaSpectrometerDetector.C \
		src/THaHRS.C src/THaLeftHRS.C src/THaRightHRS.C \
                src/THaDecData.C src/THaOutput.C src/THaString.C \
		src/THaTrackingDetector.C src/THaNonTrackingDetector.C \
		src/THaPidDetector.C src/THaSubDetector.C \
		src/THaAnalysisObject.C src/THaDetectorBase.C src/THaRTTI.C \
		src/THaPhysicsModule.C src/THaVertexModule.C \
		src/THaAnalyzer.C src/THaPrintOption.C \
		src/THaBeam.C src/THaIdealBeam.C \
		src/THaTrack.C src/THaPIDinfo.C src/THaParticleInfo.C \
		src/THaCluster.C src/THaMatrix.C src/THaArrayString.C \
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
		src/THaTwoarmVertex.C src/THaAvgVertex.C \
		src/THaExtTarCor.C src/THaDebugModule.C

OBJ           = $(SRC:.C=.o)
HDR           = $(SRC:.C=.h) src/THaGlobals.h src/VarDef.h src/VarType.h
DEP           = $(SRC:.C=.d)
OBJS          = $(OBJ) haDict.o

LIBHALLA      = libHallA.so
PROGRAMS      = analyzer

all:            $(PROGRAMS)

$(LIBHALLA):	$(OBJS)
		$(LD) $(LDFLAGS) $(SOFLAGS) -o $@ $^
		@echo "$@ done"

analyzer:	src/main.o $(LIBHALLA) libdc.so libscaler.so
		$(LD) $(LDFLAGS) src/main.o $(HALLALIBS) $(GLIBS) -o $@

libdc.so:
		$(MAKE) -C $(DCDIR)

libscaler.so:
		$(MAKE) -C $(SCALERDIR)
		rm -f DB/scaler.map
		cp $(SCALERDIR)/scaler.map DB/

clean:
		$(MAKE) -C $(DCDIR) clean
		$(MAKE) -C $(SCALERDIR) clean
		rm -f *.so *.a $(PROGRAMS) *.o *Dict.* *~
		cd src; rm -f *.o *~

realclean:	clean
		$(MAKE) -C $(DCDIR) realclean
		$(MAKE) -C $(SCALERDIR) realclean
		find . -name "*.d" -exec rm {} \;

srcdist:	realclean
		gtar czvf ../v$(VERSION).tar.gz -X .exclude \
			.exclude src examples DB $(DCDIR) $(SCALERDIR) \
			Makefile RELEASE_NOTES* 

cvsdist:	srcdist
		gunzip ../v$(VERSION).tar.gz
		gtar rvf ../v$(VERSION).tar CVS */CVS
		gzip ../v$(VERSION).tar

haDict.C: $(HDR) src/HallA_LinkDef.h
	@echo "Generating dictionary haDict..."
	$(ROOTSYS)/bin/rootcint -f $@ -c $(INCLUDES) $(HDR) \
		src/HallA_LinkDef.h


###--- DO NOT CHANGE ANYTHING BELOW THIS LINE UNLESS YOU KNOW WHAT 
###    YOU ARE DOING

.SUFFIXES:
.SUFFIXES: .c .cc .cpp .C .o .d

%.o:	%.C
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.d:	%.C
	@echo Creating dependencies for $<
#	@$(SHELL) -ec '$(CXX) -MM $(CXXFLAGS) -c $< \
#		| sed '\''s%\($*\)\.o[ :]*%\1.o $@ : %g'\'' > $@; \
#		[ -s $@ ] || rm -f $@'
	@$(SHELL) -ec '$(MAKEDEPEND) -MM $(INCLUDES) -c $< \
		| sed '\''s%^.*\.o%$*\.o%g'\'' \
		| sed '\''s%\($*\)\.o[ :]*%\1.o $@ : %g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

###

-include $(DEP)

