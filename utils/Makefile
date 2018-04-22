all: dbconvert

ifdef DEBUG
  CXXFLAGS    = -g -O0 -Wall -Wextra
else
  CXXFLAGS    = -g -O2 -Wall -Wextra
endif

INCDIRS     = ../src ../hana_decode
INCLUDES    = $(addprefix -I, $(INCDIRS))

CXX         = $(shell root-config --cxx)
ROOTCFLAGS  = $(shell root-config --cflags)
LD          = $(shell root-config --ld)
LDFLAGS     = $(shell root-config --ldflags)

CXXFLAGS   += $(ROOTCFLAGS) $(INCLUDES)
#ALLINCLUDES  = -I$(shell root-config --incdir) $(INCLUDES)
LIBS        = $(shell root-config --libs) -L.. -lHallA -ldc

# Add EVIO lib, needed by libdc
ifndef EVIO_LIBDIR
  EVIO_LIBDIR = ..
endif
LIBS += -L$(EVIO_LIBDIR) -levio

dbconvert:	dbconvert.o
		$(LD) $(LDFLAGS) $(LIBS) -o $@ $^

clean:
		rm -f dbconvert dbconvert.o

%.o:		%.cxx Makefile
		$(CXX) $(CXXFLAGS) -o $@ -c $<

.PHONY: all clean
