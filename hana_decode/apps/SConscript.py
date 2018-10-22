###### Hall A Decoder Test Executables SConscript File #####
###### Ole Hansen (ole@jlab.org) Oct 2018

import os
Import ('baseenv')

proceed = "1" or "y" or "yes" or "Yes" or "Y"
if baseenv.subst('$STANDALONE')==proceed or baseenv.GetOption('clean') \
    or 'uninstall' in COMMAND_LINE_TARGETS:

    thisdir_fullpath = Dir('.').path
    thisdir = os.path.basename(os.path.normpath(thisdir_fullpath))

    # Executables
    appnames = Split("""
    tstfadc tstfadcblk tstf1tdc tstio tstoo tdecpr prfact epicsd tdecex tst1190
    """)

    # Configure environment. We need to build against EVIO
    env = baseenv.Clone()
    env.Append(CPPPATH = baseenv.subst('$EVIO_INC'))
    env.Append(LIBS = ['evio'])
    env.Append(LIBPATH = [baseenv.subst('$EVIO_LIB')])
    env.Append(RPATH = [baseenv.subst('$EVIO_LIB')])
    # SCons seems to ignore $RPATH on macOS... sigh
    if env['PLATFORM'] == 'darwin':
        try:
            for rp in env['RPATH']:
                env.Append(LINKFLAGS = ['-Wl,-rpath,'+rp])
        except KeyError:
            pass

    apps = []
    sources = []
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
