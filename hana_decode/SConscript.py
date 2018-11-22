###### Hall A Software decoder library SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013
###### Modified for Podd 1.7 directory layout: Ole Hansen (ole@jlab.org) Sep/Oct 2018

import os
from configure import FindEVIO
from podd_util import build_library
Import('baseenv')

libname = 'dc'
altname = 'haDecode'

src = """
Caen1190Module.cxx
Caen775Module.cxx
Caen792Module.cxx
CodaDecoder.cxx
F1TDCModule.cxx
Fadc250Module.cxx
FastbusModule.cxx
GenScaler.cxx
Lecroy1875Module.cxx
Lecroy1877Module.cxx
Lecroy1881Module.cxx
Module.cxx
PipeliningModule.cxx
Scaler1151.cxx
Scaler3800.cxx
Scaler3801.cxx
Scaler560.cxx
THaCodaData.cxx
THaCodaFile.cxx
THaCrateMap.cxx
THaEpics.cxx
THaEvData.cxx
THaSlotData.cxx
THaUsrstrutils.cxx
VmeModule.cxx
"""

dcenv = baseenv.Clone()

# Find/build EVIO and configure the decoder build environment for it
FindEVIO(dcenv)
local_evio = (dcenv['LOCAL_EVIO'] == 1)
evioname = 'evio'
eviolib = dcenv.subst('$SHLIBPREFIX')+evioname+dcenv.subst('$SHLIBSUFFIX')
dcenv.Append(CPPPATH = dcenv.subst('$EVIO_INC'))
dcenv.Replace(LIBS = evioname, LIBPATH = dcenv.subst('$EVIO_LIB'),
              RPATH = [dcenv.subst('$EVIO_LIB')])
if local_evio:
    dc_install_rpath = []  # analyzer already contains the installation libdir
else:
    dc_install_rpath = dcenv['RPATH']

# Decoder library
dclib = build_library(dcenv, libname, src,
                      extrahdrs = ['Decoder.h'],
                      extradicthdrs = ['THaBenchmark.h'],
                      dictname = altname,
                      install_rpath = dc_install_rpath,
                      versioned = True
                      )

proceed = "1" or "y" or "yes" or "Yes" or "Y"
if dcenv.subst('$STANDALONE')==proceed or dcenv.GetOption('clean') \
    or 'uninstall' in COMMAND_LINE_TARGETS:
    SConscript(dirs = ['apps'], name='SConscript.py', exports='dcenv')

# Needed for locally built EVIO library
# (Versioning in build_library sets SHLIBSUFFIX, so plain libevio.so is no longer found ... sigh)
if local_evio:
    dcenv.Depends(dclib, os.path.join(dcenv.subst('$EVIO_LIB'), eviolib))
