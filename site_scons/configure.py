# configure.py
# Configuration utilities for Hall A C++ analyzer

import sys
import os
import platform
import subprocess

# Platform configuration
def config(env,args):

    if env['PLATFORM'] == 'posix':
        if (platform.machine() == 'x86_64'):
            print ("Got a 64-bit processor, I can do a 64-bit build in theory...")
            for element in platform.architecture():
                if (element == '32bit'):
                    print ('32-bit Linux build')
                    env['MEMORYMODEL'] = '32bit'
                    import linux32
                    linux32.config(env, args)
                    break
                elif (element == '64bit'):
                    print ('64-bit Linux build')
                    env['MEMORYMODEL'] = '64bit'
                    import linux64
                    linux64.config(env, args)
                    break
                else:
                    print ('Memory model not specified, so I\'m building 32-bit...')
                    env['MEMORYMODEL'] = '32bit'
                    import linux32
                    linux32.config(env, args)
        else:
            print ('32-bit Linux Build.')
            env['MEMORYMODEL'] = '32bit'
            import linux32
            linux32.config(env, args)
    elif env['PLATFORM'] == 'darwin':
        print ('OS X Darwin is a 64-bit build.')
        env['MEMORYMODEL'] = '64bit'
        import darwin64
        darwin64.config(env, args)
    else:
        print ('ERROR! unrecognized platform.  Twonk.')

# which() utility
def which(program):
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
    return None

####### ROOT Definitions ####################
def FindROOT(env, need_glibs = True):
    root_config = 'root-config'
    try:
        env.PrependENVPath('PATH',env['ENV']['ROOTSYS'] + '/bin')
    except KeyError:
        pass    # ROOTSYS not defined

    try:
        if need_glibs:
            env.ParseConfig(root_config + ' --cflags --glibs')
        else:
            env.ParseConfig(root_config + ' --cflags --libs')
        if sys.version_info >= (3, 0):
            cmd = root_config + ' --cxx'
            env.Replace(CXX = os.fsdecode(subprocess.check_output(cmd, shell=True).rstrip()))
            cmd = root_config + ' --version'
            env.Replace(ROOTVERS = os.fsdecode(subprocess.check_output(cmd, shell=True).rstrip()))
            print ('CXX value Version 3 = ',env['CXX'])
            print ('ROOTVERS value Version 3 = ',env['ROOTVERS'])
        if sys.version_info >= (2, 7) and sys.version_info < (3, 0):
            cmd = root_config + ' --cxx'
            env.Replace(CXX = subprocess.check_output(cmd, shell=True).rstrip())
            cmd = root_config + ' --version'
            env.Replace(ROOTVERS = subprocess.check_output(cmd, shell=True).rstrip())
            print ('CXX value Version 2 = ',env['CXX'])
            print ('ROOTVERS value Version 2 = ',env['ROOTVERS'])
        if sys.version_info < (2, 7):
            env.Replace(CXX = subprocess.Popen([root_config, '--cxx'],\
                stdout=subprocess.PIPE).communicate()[0].rstrip())
            env.Replace(ROOTVERS = subprocess.Popen([root_config,\
                '--version'],stdout=subprocess.PIPE).communicate()[0].rstrip())
            print ('CXX value Version 1 = ',env['CXX'])
            print ('ROOTVERS value Version 1 = ',env['ROOTVERS'])
        if platform.system() == 'Darwin':
            try:
                env.Replace(LINKFLAGS = env['LINKFLAGS'].remove('-pthread'))
            except:
                pass #  '-pthread' was not present in LINKFLAGS

    except OSError:
        print('!!! Cannot find ROOT.  Check if root-config is in your PATH.')
        env.Exit(1)

# EVIO environment
def FindEVIO(env, build_it = True, fail_if_missing = True):
    uname = os.uname();
    platform = uname[0];
    machine = uname[4];
    if platform == 'Linux':
        sosuf = 'so'
    elif platform == 'Darwin':
        sosuf = 'dylib'
    evio_header_file = 'evio.h'
    evio_library_file = 'libevio.'+ sosuf

    evio_inc_dir = os.getenv('EVIO_INCDIR')
    evio_lib_dir = os.getenv('EVIO_LIBDIR')
    th1 = env.FindFile(evio_header_file,evio_inc_dir)
    tl1 = env.FindFile(evio_library_file,evio_lib_dir)
    #print ('%-12s' % ('%s:' % evio_header_file), FindFile(evio_header_file, evio_inc_dir))
    #print ('%-12s' % ('%s:' % evio_library_file), FindFile(evio_library_file, evio_lib_dir))
    #
    evio_dir = os.getenv('EVIO')
    evio_arch = platform + '-' + machine
    if evio_dir is not None:
        evio_lib_dir2 = evio_dir + '/' + evio_arch + '/lib'
        evio_inc_dir2 = evio_dir + '/' + evio_arch + '/include'
        th2 = env.FindFile(evio_header_file,evio_inc_dir2)
        tl2 = env.FindFile(evio_library_file,evio_lib_dir2)
    else:
        evio_lib_dir2 = None
        evio_inc_dir2 = None
        th2 = None
        tl2 = None
    #print ('%-12s' % ('%s:' % evio_header_file), FindFile(evio_header_file, evio_inc_dir2))
    #print ('%-12s' % ('%s:' % evio_library_file), FindFile(evio_library_file, evio_lib_dir2))
    #
    coda_dir = os.getenv('CODA')
    if coda_dir is not None:
        evio_lib_dir3 = coda_dir + '/' + evio_arch + '/lib'
        evio_inc_dir3 = coda_dir + '/' + evio_arch + '/include'
        th3 = env.FindFile(evio_header_file,evio_inc_dir3)
        tl3 = env.FindFile(evio_library_file,evio_lib_dir3)
    else:
        evio_lib_dir3 = None
        evio_inc_dir3 = None
        th3 = None
        tl3 = None
        #print ('%-12s' % ('%s:' % evio_header_file), FindFile(evio_header_file, evio_inc_dir3))
        #print ('%-12s' % ('%s:' % evio_library_file), FindFile(evio_library_file, evio_lib_dir3))
        #
    if th1 is not None and tl1 is not None:
        env.Append(EVIO_LIB = evio_lib_dir)
        env.Append(EVIO_INC = evio_inc_dir)
    elif th2 is not None and tl2 is not None:
        env.Append(EVIO_LIB = evio_lib_dir2)
        env.Append(EVIO_INC = evio_inc_dir2)
    elif th3 is not None and tl3 is not None:
        env.Append(EVIO_LIB = evio_lib_dir3)
        env.Append(EVIO_INC = evio_inc_dir3)
    elif build_it:
        print ("No external EVIO environment configured !!!")
        print ("Using local installation ... ")
        evio_version = '4.4.6'
        evio_local = env.subst('$HA_DIR')+'/evio'
        evio_local_lib = "%s/evio-%s/src/libsrc/.%s" % (evio_local,evio_version,evio_arch)
        evio_local_inc = "%s/evio-%s/src/libsrc" % (evio_local,evio_version)
        evio_tarfile = "%s/master.tar.gz" % (evio_local)
        evio_command_dircreate = "mkdir -p %s" % (evio_local)
        os.system(evio_command_dircreate)

        ####### Check to see if scons -c has been called #########
        if env.GetOption('clean'):
            subprocess.call(['echo', '!!!!!!!!!!!!!! EVIO Cleaning Process !!!!!!!!!!!! '])
            evio_command_cleanup = "rm -f libevio*.*; cd %s; rm -rf evio-%s" % (evio_local,evio_version)
            print ("evio_command_cleanup = %s" % evio_command_cleanup)
            os.system(evio_command_cleanup)
        else:
            if not os.path.isdir(evio_local_lib):
                if not os.path.exists(evio_tarfile):
                    evio_command_download = "cd %s; curl -LO https://github.com/JeffersonLab/hallac_evio/archive/master.tar.gz" % (evio_local)
                    print ("evio_command_download = %s" % evio_command_download)
                    os.system(evio_command_download)

                evio_command_unpack = "cd %s; tar xvfz master.tar.gz; mv hallac_evio-master evio-%s" % (evio_local,evio_version)
                print ("evio_command_unpack = %s" % evio_command_unpack)
                os.system(evio_command_unpack)

            evio_command_scons = "cd %s/evio-%s; scons src/libsrc/.%s/" % (evio_local,evio_version,evio_arch)
            print ("evio_command_scons = %s" % evio_command_scons)
            os.system(evio_command_scons)
            evio_local_lib_files = "%s/libevio.*" % (evio_local_lib)
            evio_command_libcopy = "cp -vf %s ." % (evio_local_lib_files)
            print ("evio_command_libcopy = %s" % evio_command_libcopy)
            os.system(evio_command_libcopy)

        env.Append(EVIO_LIB = evio_local_lib)
        env.Append(EVIO_INC = evio_local_inc)
    elif fail_if_missing:
        print('!!! Cannot find EVIO library.  Set EVIO_LIBDIR/EVIO_INCDIR, EVIO or CODA.')
        env.Exit(1)
    else:
        return

    print ("EVIO lib Directory = %s" % env.subst('$EVIO_LIB'))
    print ("EVIO include Directory = %s" % env.subst('$EVIO_INC'))
    env.Append(CPPPATH = ['$EVIO_INC'])

#end configure.py
