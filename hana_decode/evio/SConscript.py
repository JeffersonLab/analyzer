###### Hall A local EVIO SConscript File #####

import os
Import('baseenv')

libname = 'evio'

src = Glob('*.c')
hdr = Glob('*.h')

evioenv = baseenv.Clone()
evioenv.Replace(CPPPATH = [], CPPDEFINES = [], LIBS = [], LIBPATH = [], RPATH = [])
debug = ARGUMENTS.get('debug',0)
if int(debug):
    evioenv.Append(CCFLAGS = ['-g', '-O0'])
else:
    evioenv.Append(CCFLAGS = '-O')

libname_soname = evioenv.subst('$SHLIBPREFIX')+libname+evioenv.subst('$SHLIBSUFFIX')
pf = evioenv['PLATFORM']
if pf == 'posix':
    evioenv.Append(SHLINKFLAGS = '-Wl,-soname='+libname_soname)
elif pf == 'darwin':
    evioenv.Append(SHLINKFLAGS = '-Wl,-install_name,'+'@rpath/'+libname_soname)

eviolib = evioenv.SharedLibrary(target = libname, source = src)

thisdir_fullpath = evioenv.Dir('.').path
#thisdir = os.path.basename(os.path.normpath(thisdir_fullpath))
evioenv.Install(os.path.join(evioenv.subst('$INSTALLDIR'),evioenv.subst('$LIBSUBDIR')), eviolib)
evioenv.Install(os.path.join(evioenv.subst('$INSTALLDIR'),'include'), hdr)
evioenv.Install(os.path.join(evioenv.subst('$INSTALLDIR'),'src',thisdir_fullpath), src)
