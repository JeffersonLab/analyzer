# podd_utils.py
# Utility functions for Podd SCons build

import os
from SCons.Action import ActionFactory
from SCons.Script.SConscript import SConsEnvironment

SConsEnvironment.OSCommand = ActionFactory(os.system,
            lambda command : 'os.system("%s")' % command)

import SCons.Util

def list_to_path(lst):
    result = ''
    if SCons.Util.is_List(lst):
        for element in lst:
            result += str(element)
            if len(lst) > 1 and element != lst[-1]:
                result += ':'
    else:
        result = lst
    return result

def InstallWithRPATH(env, dest, files, rpath):
    obj = env.Install(dest, files)
    if env['PLATFORM'] == 'posix':
        rpathstr = list_to_path(rpath)
        if env.WhereIs('patchelf'):
            if env.subst('$ADD_INSTALL_RPATH') and rpathstr:
                patch_cmd = "patchelf --force-rpath --set-rpath '"+rpathstr+"' "
            else:
                patch_cmd = "patchelf --remove-rpath "
            for i in obj:
                env.AddPostAction(i, env.OSCommand(patch_cmd+str(i)))
        elif env.WhereIs('chrpath') \
                and not (env.subst('$ADD_INSTALL_RPATH') and rpathstr):
            # chrpath can only reliably delete RPATH
            for i in obj:
                env.AddPostAction(i, env.OSCommand('chrpath -d '+str(i)))
        else:
            print('WARNING: patchelf not found, cannot set RPATH')
    elif env['PLATFORM'] == 'darwin':
        for i in obj:
            cmd = os.path.join(env.subst('$HA_DIR'),"site_scons","clear_macos_rpath.sh")
            env.AddPostAction(i, env.OSCommand(cmd+" "+str(i)))
            if env.subst('$ADD_INSTALL_RPATH') and rpath:
                tool_cmd = "install_name_tool"
                # rpath could contain empty strings or be ['']
                add_to_cmd = ''
                for rp in rpath:
                    if rp:
                        add_to_cmd += " -add_rpath "+str(rp)
                if add_to_cmd:
                    tool_cmd += add_to_cmd+" "+str(i)
                    env.AddPostAction(i, env.OSCommand(tool_cmd))
    return obj

def create_uninstall_target(env, path, is_glob = False):
    if is_glob:
        all_files = env.Glob(path,strings=True)
        for filei in all_files:
            print('Delete(%s)' % filei)
            env.Alias("uninstall", os.remove(filei))
    else:
        print('Delete(%s)' % path)
        if os.path.exists(path):
            env.Alias("uninstall", os.remove(path))

import SCons.Script
import re

def build_library(env, sotarget, src, extrahdrs = [], extradicthdrs = [],
                  dictname = None, useenv = True, versioned = False,
                  install_rpath = [], install_srcdir = None):
    """
    Build shared library lib<sotarget> of ROOT classes from given sources "src"
    (space separated string). For each .cxx source file, a .h header file is
    expected.

    A ROOT dictionary is generated from the headers and compiled into the library.
    The optional "extradicthdrs" headers will be added to the list of headers used
    for generating the dictionary.

    If the dictionary source file should have a different name than <sotarget>Dict.cxx,
    specify that name as "dictname". The dictionary will expect a link definition file
    <dictname>_LinkDef.h.

    All sources and corresponding headers and the "extradicthdrs" will be installed
    in the source and include file installation locations. If any additional headers
    should be installed, specify them in "extrahdrs". "extrahdrs" will not passed to
    the ROOT dictionary generator, which is useful e.g. for non-ROOT class standalone
    headers such as local utility classes or global definitions.

    If "useenv" is True, the library will link against libraries in $LIB and search for
    libraires in $LIBPATH. Otherwise, no external libraries will be linked.
    Other environment variables (compiler flags, include directives, RPATH etc.)
    are not affected by this flag

    "install_rpath" is a list of directories that will be set on the installed library
    if RPATH installation is globally enabled with $ADD_INSTALL_RPATH. Literal "$" signs
    in any of these list elements (e.g. $ORIGIN) need to be given as "$$" (e.g. $$ORIGIN).
    """

    # Current location relative to top directory
    thisdir_fullpath = env.Dir('.').path
    thisdir = os.path.basename(os.path.normpath(thisdir_fullpath))

    if not install_srcdir:
        install_srcdir = thisdir
    if not dictname:
        dictname = sotarget

    if useenv:
        linklibs = env['LIBS']
        linklibpath = env['LIBPATH']
    else:
        linklibs = ['']
        linklibpath = ['']

    # Sources and headers
    srclist = env.Split(src)
    hdr = re.sub(r'\.cxx','.h',src)

    installhdrs = env.Split(hdr)
    installhdrs.extend(extradicthdrs)
    installhdrs.extend(extrahdrs)

    dicthdrs = env.Split(hdr)
    dicthdrs.extend(extradicthdrs)
    dicthdrs.append(dictname+'_LinkDef.h')

    # ROOT dictionary for this library
    rootdict = dictname+'Dict.cxx'
    libbase = env.subst('$SHLIBPREFIX')+sotarget
    thedict = env.RootCint(rootdict, dicthdrs, PCMNAME=libbase)

    # Versioned shared library symlink names
    libname_so = libbase+env.subst('$SHLIBSUFFIX')
    if env['PLATFORM'] == 'posix':
        # Linux
        if versioned:
            libname_soname = libname_so+'.'+env.subst('$SOVERSION')
            libname_versioned = libname_so+'.'+env.subst('$VERSION')
            shlibsuffix = env.subst('$SHLIBSUFFIX')+'.'+env.subst('$VERSION')
        else:
            libname_soname = libname_so
            shlibsuffix = env.subst('$SHLIBSUFFIX')
        shlinkflags = ['-Wl,-soname='+libname_soname]
    elif env['PLATFORM'] == 'darwin':
        # macOS
        if versioned:
            libname_soname = libbase+'.'+env.subst('$SOVERSION')+env.subst('$SHLIBSUFFIX')
            libname_versioned = libbase+'.'+env.subst('$VERSION')+env.subst('$SHLIBSUFFIX')
            shlibsuffix = '.'+env.subst('$VERSION')+env.subst('$SHLIBSUFFIX')
        else:
            libname_soname = libname_so
            shlibsuffix = env.subst('$SHLIBSUFFIX')
        shlinkflags = ['-Wl,-install_name,'+'@rpath/'+libname_soname]
        if versioned:
            shlinkflags.append('-Wl,-compatibility_version,'+env.subst('$SOVERSION'))
            shlinkflags.append('-Wl,-current_version,'+env.subst('$VERSION'))
        try:
            for rp in env['RPATH']:
                shlinkflags.append('-Wl,-rpath,'+rp)
        except KeyError:
            pass
    else:
        print('build_library: Error: unsupported platform')
        Exit(3)

    # Build the library
    thislib = env.SharedLibrary(target = sotarget,
                 source = srclist+[rootdict],
                 LIBS = linklibs, LIBPATH = linklibpath,
                 SHLIBSUFFIX = shlibsuffix,
                 SONAME = libname_soname,
                 SHLINKFLAGS = env['SHLINKFLAGS']+shlinkflags)

    if versioned:
        # Create symlinks
        env.SymLink(libname_soname,thislib)
        env.SymLink(libname_so,libname_soname)

    # Installation
    install_prefix = env.subst('$INSTALLDIR')
    lib_dir = os.path.join(install_prefix,env.subst('$LIBSUBDIR'))
    #bin_dir = os.path.join(install_prefix,'bin')
    inc_dir = os.path.join(install_prefix,'include')
    src_dir = os.path.join(install_prefix,'src',install_srcdir)

    InstallWithRPATH(env,lib_dir,thislib,install_rpath)
    # Install PCM file generated by RootCint, if any
    if len(thedict) > 1:
        env.Install(lib_dir,thedict[1])
        # Install rootmap file, if any
        if len(thedict) > 2:
            env.Install(lib_dir,thedict[2])
    env.Install(inc_dir,installhdrs)
    env.Install(src_dir,srclist)

    libname_so_installpath = os.path.join(lib_dir,libname_so)
    if versioned:
        libname_soname_installpath = os.path.join(lib_dir,libname_soname)
        libname_versioned_installpath = os.path.join(lib_dir,libname_versioned)
        #Kludge for SCons's inability to install symlinks
        env.SymLink(libname_soname_installpath,libname_versioned_installpath)
        env.SymLink(libname_so_installpath,libname_soname_installpath)

        if 'uninstall' in SCons.Script.COMMAND_LINE_TARGETS:
            create_uninstall_target(env, libname_so_installpath)
            create_uninstall_target(env, libname_soname_installpath)

    return thislib

import sys
import subprocess
import platform
import time

def write_compiledata(env, compiledata):
    if sys.version_info >= (2, 7):
        try:
            cmd = "git describe --tags --always --long --dirty 2>/dev/null"
            gitrev = subprocess.check_output(cmd, shell=True).rstrip()
        except:
            gitrev = ''
        try:
            cmd = "git show --no-patch --format=%cD HEAD 2>/dev/null"
            gitdate = subprocess.check_output(cmd, shell=True).rstrip()
        except:
            gitdate = ''
        try:
            cmd = env.subst('$CXX') + " --version 2>/dev/null | head -1"
            cxxver = subprocess.check_output(cmd, shell=True).rstrip()
        except:
            cxxver = ''
        # subprocess gives us byte string literals in Python 3, but we'd like
        # Unicode strings
        if sys.version_info >= (3, 0):
            gitrev = gitrev.decode()
            gitdate = gitdate.decode()
            cxxver = cxxver.decode()
    else:
        fnull = open(os.devnull, 'w')
        try:
            gitrev = subprocess.Popen(['git', 'describe', '--tags', '--always',
                                       '--long', '--dirty', '2>dev/null'],
                        stdout=subprocess.PIPE, stderr=fnull).communicate()[0].rstrip()
        except:
            gitrev =''
        try:
            gitdate = subprocess.Popen(['git', 'describe', '--tags', '--always',
                                        '--long', '--dirty', '2>dev/null'],
                                        stdout=subprocess.PIPE, stderr=fnull).communicate()[0].rstrip()
        except:
            gitdate =''
        try:
            outp = subprocess.Popen([env.subst('$CXX'), '--version'],
                                    stdout=subprocess.PIPE, stderr=fnull).communicate()[0]
            lines = outp.splitlines()
            cxxver = lines[0]
        except:
            cxxver = ''

    f=open(compiledata,'w')
    f.write('#ifndef ANALYZER_COMPILEDATA_H\n')
    f.write('#define ANALYZER_COMPILEDATA_H\n')
    f.write('\n')
    f.write('#define HA_INCLUDEPATH "%s %s %s"\n' %
            (env.subst('$HA_HallA'), env.subst('$HA_Podd'), env.subst('$HA_DC')))
    f.write('#define HA_VERSION "%s"\n' % env.subst('$HA_VERSION'))
    if gitdate:
        f.write('#define HA_SOURCETIME "%s"\n' % gitdate)
    else:
        # Unknown git date (probably building from tarball)
        f.write('#define HA_SOURCETIME "%s"\n' % time.strftime("%a, %d %b %Y %H:%M:%S %z"))
    f.write('#define HA_OSVERS "%s"\n' % platform.platform(terse=1))
    f.write('#define HA_PLATFORM "%s-%s-%s"\n' % (platform.system(), platform.release(), platform.machine()))
    f.write('#define HA_BUILDTIME "%s"\n' % time.strftime("%a, %d %b %Y %H:%M:%S %z"))
    f.write('#define HA_BUILDNODE "%s"\n' % platform.node())
    f.write('#define HA_BUILDDIR "%s"\n' % os.getcwd())
    try:
        builduser = env['ENV']['LOGNAME']
    except:
        builduser = ''
    f.write('#define HA_BUILDUSER "%s"\n' % builduser)
    f.write('#define HA_GITREV "%s"\n' % gitrev)
    f.write('#define HA_CXXVERS "%s"\n' % cxxver)
    if cxxver:
        fields = cxxver.split()
        if len(fields) >= 2:
            if fields[0] == 'Apple':
                cxxver = fields[-1][1:-1]
            elif fields[0] == 'clang':
                cxxver = fields[0] + '-' + fields[2]
            elif fields[0] == 'g++':
                i=1
                while i<len(fields) and fields[i][-1] != ')':
                    i += 1
                if i+1 < len(fields):
                    cxxver = fields[0] + '-' + fields[i+1]
    f.write('#define HA_CXXSHORTVERS "%s"\n' % cxxver)
    f.write('#define HA_ROOTVERS "%s"\n' % env.subst('$ROOTVERS'))
    f.write('\n')
    f.write('#define ANALYZER_VERSION_CODE %s\n' % env.subst('$VERCODE'))
    f.write('#define ANALYZER_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))\n')
    f.write('#endif\n')
    f.close()
