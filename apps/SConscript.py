###### Hall A Software Main SConscript File #####
###### Author:    Edward Brash (brash@jlab.org) June 2013
###### Modified for Podd 1.7 directory layout: Ole Hansen (ole@jlab.org) Sep 2018

import os
Import('baseenv')

thisdir_fullpath = Dir('.').path
thisdir = os.path.basename(os.path.normpath(thisdir_fullpath))

# Executables
appnames = ['analyzer', 'dbconvert']
apps = []
sources = []
# SCons seems to ignore $RPATH on macOS... sigh
env = baseenv.Clone()
if env['PLATFORM'] == 'darwin':
    try:
        for rp in env['RPATH']:
            env.Append(LINKFLAGS = ['-Wl,-rpath,'+rp])
    except KeyError:
        pass

for a in appnames:
    app = env.Program(target = a, source = a+'.cxx')
    apps.append(app)
    sources.append(a+'.cxx')

# Installation
install_prefix = env.subst('$INSTALLDIR')
bin_dir = os.path.join(install_prefix,'bin')
#lib_dir = os.path.join(install_prefix,env.subst('$LIBSUBDIR'))
rel_lib_dir = os.path.join(env['RPATH_ORIGIN_TAG'],
                           os.path.join('..',env.subst('$LIBSUBDIR')))
src_dir = os.path.join(install_prefix,'src',thisdir)

env.InstallWithRPATH(bin_dir,apps,[rel_lib_dir])
env.Install(src_dir,sources)
