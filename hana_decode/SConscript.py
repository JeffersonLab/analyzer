###### Hall A Software hana_decode SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013

import os
from configure import FindEVIO
from podd_util import build_library
Import('baseenv')

libname = 'dc'
altname = 'haDecode'

src = """
Caen1190Module.C
Caen775Module.C
Caen792Module.C
CodaDecoder.C
F1TDCModule.C
Fadc250Module.C
FastbusModule.C
GenScaler.C
Lecroy1875Module.C
Lecroy1877Module.C
Lecroy1881Module.C
Module.C
PipeliningModule.C
Scaler1151.C
Scaler3800.C
Scaler3801.C
Scaler560.C
SimDecoder.C
THaCodaData.C
THaCodaDecoder.C
THaCodaFile.C
THaCrateMap.C
THaEpics.C
THaEvData.C
THaFastBusWord.C
THaSlotData.C
THaUsrstrutils.C
VmeModule.C
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
                      versioned = True,
                      destdir = dcenv.subst('$HA_DIR')
                      )

# Needed for locally built EVIO library
# (Versioning in build_library sets SHLIBSUFFIX, so plain libevio.so is no
# longer found ... sigh)
if local_evio:
    dcenv.Depends(dclib, os.path.join(dcenv.subst('$EVIO_LIB'), eviolib))

# ----- Standalone executables -----
proceed = "1" or "y" or "yes" or "Yes" or "Y"
if dcenv.subst('$STANDALONE')==proceed or dcenv.GetOption('clean') \
    or 'uninstall' in COMMAND_LINE_TARGETS:

    thisdir_fullpath = Dir('.').path
    thisdir = os.path.basename(os.path.normpath(thisdir_fullpath))

    # Executables
    appnames = ['tstfadc', 'tstfadcblk', 'tstf1tdc', 'tstio',
                'tstoo', 'tdecpr', 'prfact', 'epicsd', 'tdecex',
                'tst1190']
    apps = []
    sources = []
    env = baseenv.Clone()
    # SCons seems to ignore $RPATH on macOS... sigh
    if env['PLATFORM'] == 'darwin':
        try:
            for rp in env['RPATH']:
                env.Append(LINKFLAGS = ['-Wl,-rpath,'+rp])
        except KeyError:
            pass

    for a in appnames:
        if a == 'epicsd':
            src = ['epics_main.C']
        else:
            src = [a+'_main.C']
            if a == 'tdecex':
                src.append('THaGenDetTest.C')

        app = env.Program(target = a, source = src)
        apps.append(app)
        sources.append(src)

        # Installation
        install_prefix = env.subst('$INSTALLDIR')
        bin_dir = os.path.join(install_prefix,'bin')
        #lib_dir = os.path.join(install_prefix,env.subst('$LIBSUBDIR'))
        rel_lib_dir = os.path.join(env['RPATH_ORIGIN_TAG'],
                                   os.path.join('..',env.subst('$LIBSUBDIR')))
        src_dir = os.path.join(install_prefix,'src',thisdir)

        env.InstallWithRPATH(bin_dir,apps,[rel_lib_dir])
        env.Install(src_dir,sources)
