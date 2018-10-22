###### Hall A Software decoder library SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013
###### Modified for Podd 1.7 directory layout: Ole Hansen (ole@jlab.org) Sep 2018

from podd_util import build_library
Import('baseenv')

libname = 'dc'
altname = 'haDecode'

# Requires SCons >= 2.3.5 for "exclude" keyword
#src = Glob('*.cxx',exclude=['*_main.cxx','*_onl.cxx','calc_thresh.cxx',
#                           'THaEtClient.cxx','THaGenDetTest.cxx'])

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
dcenv.Append(CPPPATH = baseenv.subst('$EVIO_INC'))
dcenv.Replace(LIBS = ['evio'])
dcenv.Replace(LIBPATH = [baseenv.subst('$EVIO_LIB')])
dcenv.Replace(RPATH = [baseenv.subst('$EVIO_LIB')])

build_library(dcenv, libname, src,
              extrahdrs = ['Decoder.h'],
              extradicthdrs = ['THaBenchmark.h'],
              dictname = altname,
              install_rpath = dcenv.subst('$RPATH')
              )
