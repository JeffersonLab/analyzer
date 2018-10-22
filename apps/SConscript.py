###### Hall A Software Main SConscript File #####
###### Author:    Edward Brash (brash@jlab.org) June 2013
###### Modified for Podd 1.7 directory layout: Ole Hansen (ole@jlab.org) Sep 2018

import os
Import ('baseenv')

thisdir_fullpath = Dir('.').path
thisdir = os.path.basename(os.path.normpath(thisdir_fullpath))

# Executables
appnames = ['analyzer', 'dbconvert']
apps = []
sources = []
for a in appnames:
    app = baseenv.Program(target = a, source = a+'.cxx')
    apps.append(app)
    sources.append(a+'.cxx')

# Installation
install_prefix = baseenv.subst('$INSTALLDIR')
bin_dir = os.path.join(install_prefix,'bin')
#lib_dir = os.path.join(install_prefix,'lib')
rel_lib_dir = '$$'+os.path.join('ORIGIN',os.path.join('..',baseenv.subst('$LIBSUBDIR')))
src_dir = os.path.join(install_prefix,os.path.join('src',thisdir))

baseenv.InstallWithRPATH(bin_dir,apps,rel_lib_dir)
baseenv.Install(src_dir,sources)
