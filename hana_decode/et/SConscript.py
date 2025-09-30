###### Hall A local ET SConscript File #####

import os
Import('baseenv')

libname = 'et'

src = """
et_attachment.c et_bridge.c et_common.c et_data.c et_event.c
et_init.c et_local.c et_mem.c et_network.c et_noshare.c et_openconfig.c
et_readwrite.c et_remote.c et_server.c et_statconfig.c et_station.c
et_sysconfig.c et_system.c etCommonNetwork.c et_fifo.c
"""

hdr = Glob('et*.h')

etenv = baseenv.Clone()
etenv.Replace(CPPPATH = [], CPPDEFINES = [], LIBS = [], LIBPATH = [], RPATH = [])
debug = ARGUMENTS.get('debug',0)
if int(debug):
    etenv.Append(CCFLAGS = ['-g', '-O0'])
else:
    etenv.Append(CCFLAGS = '-O')

libname_soname = etenv.subst('$SHLIBPREFIX')+libname+etenv.subst('$SHLIBSUFFIX')
pf = etenv['PLATFORM']
if pf == 'posix':
    etenv.Append(SHLINKFLAGS = '-Wl,-soname='+libname_soname)
elif pf == 'darwin':
    etenv.Append(SHLINKFLAGS = '-Wl,-install_name,'+'@rpath/'+libname_soname)

etlib = etenv.SharedLibrary(target = libname, source = src)

thisdir_fullpath = etenv.Dir('.').path
#thisdir = os.path.basename(os.path.normpath(thisdir_fullpath))
etenv.Install(os.path.join(etenv.subst('$INSTALLDIR'),etenv.subst('$LIBSUBDIR')), etlib)
etenv.Install(os.path.join(etenv.subst('$INSTALLDIR'),'include'), hdr)
etenv.Install(os.path.join(etenv.subst('$INSTALLDIR'),'src',thisdir_fullpath), src)
