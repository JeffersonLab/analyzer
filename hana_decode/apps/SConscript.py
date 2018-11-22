###### Hall A Decoder Test Executables SConscript File #####
###### Ole Hansen (ole@jlab.org) Oct 2018

import os
from configure import FindROOT
Import ('dcenv')

thisdir_fullpath = Dir('.').path
thisdir = os.path.basename(os.path.normpath(thisdir_fullpath))

# Executables
appnames = ['tstfadc', 'tstfadcblk', 'tstf1tdc', 'tstio',
            'tstoo', 'tdecpr', 'prfact', 'epicsd', 'tdecex',
            'tst1190']
apps = []
sources = []
env = dcenv.Clone()
# Link only with decoder 'libdc', not EVIO or any other libs
env.Replace(LIBS = ['dc'])
env.Replace(LIBPATH = [env.subst('$HA_DC')])
env.Replace(RPATH = [env.subst('$HA_DC')])
# Also need to link with ROOT - can be made more elegant by
# setting up a ROOT-only aware environment in parent project
FindROOT(env)
# SCons seems to ignore $RPATH on macOS... sigh
if env['PLATFORM'] == 'darwin':
    try:
        for rp in env['RPATH']:
            env.Append(LINKFLAGS = ['-Wl,-rpath,'+rp])
    except KeyError:
        pass

for a in appnames:
    if a == 'epicsd':
        src = ['epics_main.cxx']
    else:
        src = [a+'_main.cxx']
    if a == 'tdecex':
        src.append('THaGenDetTest.cxx')

    app = env.Program(target = a, source = src)
    apps.append(app)
    sources.append(src)

# Installation
install_prefix = env.subst('$INSTALLDIR')
bin_dir = os.path.join(install_prefix,'bin')
#lib_dir = os.path.join(install_prefix,'lib')
rel_lib_dir = os.path.join(env['RPATH_ORIGIN_TAG'],
                           os.path.join('..',env.subst('$LIBSUBDIR')))
src_dir = os.path.join(install_prefix,os.path.join('src',thisdir_fullpath))

env.InstallWithRPATH(bin_dir,apps,[rel_lib_dir])
env.Install(src_dir,sources)
