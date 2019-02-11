###### Hall A Software Main SConscript File #####
###### Author:    Edward Brash (brash@jlab.org) June 2013

import os
Import ('baseenv')

#thisdir_fullpath = Dir('.').path
#thisdir = os.path.basename(os.path.normpath(thisdir_fullpath))

env = baseenv.Clone()

# Executables
sources = 'src/main.C'
app = env.Program('analyzer', sources)
# SCons seems to ignore $RPATH on macOS... sigh
if env['PLATFORM'] == 'darwin':
    try:
        for rp in env['RPATH']:
            env.Append(LINKFLAGS = ['-Wl,-rpath,'+rp])
    except KeyError:
        pass

# Installation
install_prefix = env.subst('$INSTALLDIR')
bin_dir = os.path.join(install_prefix,'bin')
#lib_dir = os.path.join(install_prefix,env.subst('$LIBSUBDIR'))
rel_lib_dir = os.path.join(env['RPATH_ORIGIN_TAG'],'..',env.subst('$LIBSUBDIR'))
src_dir = os.path.join(install_prefix,'src','src')

env.InstallWithRPATH(bin_dir,app,[rel_lib_dir])
env.Install(src_dir,sources)
