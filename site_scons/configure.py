# configure.py
# Configuration utilities for Hall A C++ analyzer

import sys
import os
import platform
import subprocess
import shutil
import glob
import tarfile
try:
    import urllib2
except ImportError:
    import urllib.request as urllib2

# Platform configuration
def config(env,args):

    standalone = args.get('standalone',0)
    cppcheck = args.get('cppcheck',0)
    srcdist = args.get('srcdist',0)
    if int(standalone):
        env.Append(STANDALONE= '1')
    if int(cppcheck):
        env.Append(CPPCHECK = '1')
    if int(srcdist):
        env.Append(SRCDIST = '1')

    env.Replace(LIBSUBDIR = 'lib')
    env.Replace(RPATH_ORIGIN_TAG = '$$ORIGIN')

    if env['PLATFORM'] == 'posix':
        import linux32, linux64
        if (platform.machine() == 'x86_64'):
            print ("Got a 64-bit processor, I can do a 64-bit build in theory...")
            for element in platform.architecture():
                if (element == '32bit'):
                    print ('32-bit Linux build')
                    env['MEMORYMODEL'] = '32bit'
                    linux32.config(env, args)
                    break
                elif (element == '64bit'):
                    print ('64-bit Linux build')
                    env['MEMORYMODEL'] = '64bit'
                    linux64.config(env, args)
                    break
                else:
                    print ('Memory model not specified, so I\'m building 32-bit...')
                    env['MEMORYMODEL'] = '32bit'
                    linux32.config(env, args)
        else:
            print ('32-bit Linux Build.')
            env['MEMORYMODEL'] = '32bit'
            linux32.config(env, args)
    elif env['PLATFORM'] == 'darwin':
        import darwin64
        print ('OS X Darwin is a 64-bit build.')
        env['MEMORYMODEL'] = '64bit'
        darwin64.config(env, args)
    else:
        print ('ERROR! unrecognized platform.  Twonk.')

####### ROOT Definitions ####################
def FindROOT(env, need_glibs = True):
    root_config = 'root-config'
    try:
        rootsys = env['ENV']['ROOTSYS']
        env.PrependENVPath('PATH', os.path.join(rootsys,'bin'))
    except KeyError:
        pass    # ROOTSYS not defined

    try:
        if need_glibs:
            env.ParseConfig(root_config + ' --cflags --glibs')
        else:
            env.ParseConfig(root_config + ' --cflags --libs')
        if sys.version_info >= (3, 0):
            cmd = root_config + ' --cxx'
            env.Replace(CXX = os.fsdecode(subprocess.check_output(cmd, shell=True).rstrip()))  # @UndefinedVariable
            cmd = root_config + ' --version'
            env.Replace(ROOTVERS = os.fsdecode(subprocess.check_output(cmd, shell=True).rstrip()))  # @UndefinedVariable
            print ('CXX value Version 3 = ',env['CXX'])
            print ('ROOTVERS value Version 3 = ',env['ROOTVERS'])
        elif sys.version_info >= (2, 7) and sys.version_info < (3, 0):
            cmd = root_config + ' --cxx'
            env.Replace(CXX = subprocess.check_output(cmd, shell=True).rstrip())
            cmd = root_config + ' --version'
            env.Replace(ROOTVERS = subprocess.check_output(cmd, shell=True).rstrip())
            print ('CXX value Version 2 = ',env['CXX'])
            print ('ROOTVERS value Version 2 = ',env['ROOTVERS'])
        elif sys.version_info < (2, 7):
            env.Replace(CXX = subprocess.Popen([root_config, '--cxx'],\
                stdout=subprocess.PIPE).communicate()[0].rstrip())
            env.Replace(ROOTVERS = subprocess.Popen([root_config,\
                '--version'],stdout=subprocess.PIPE).communicate()[0].rstrip())
            print ('CXX value Version 1 = ',env['CXX'])
            print ('ROOTVERS value Version 1 = ',env['ROOTVERS'])
        if env['PLATFORM'] == 'darwin':
            try:
                env.Replace(LINKFLAGS = env['LINKFLAGS'].remove('-pthread'))
            except:
                pass #  '-pthread' was not present in LINKFLAGS

    except OSError:
        print('!!! Cannot find ROOT.  Check if root-config is in your PATH.')
        env.Exit(1)

# EVIO environment
def FindEVIO(env, build_it = True, fail_if_missing = True):
    env.Replace(LOCAL_EVIO = 0)
    uname = os.uname();
    platform = uname[0];
    machine = uname[4];
    evio_header_file = 'evio.h'
    evio_library_file = 'libevio' + env.subst('$SHLIBSUFFIX')

    evio_inc_dir = os.getenv('EVIO_INCDIR')
    evio_lib_dir = os.getenv('EVIO_LIBDIR')
    th1 = env.FindFile(evio_header_file,evio_inc_dir)
    tl1 = env.FindFile(evio_library_file,evio_lib_dir)
    #print ('%-12s' % ('%s:' % evio_header_file), FindFile(evio_header_file, evio_inc_dir))
    #print ('%-12s' % ('%s:' % evio_library_file), FindFile(evio_library_file, evio_lib_dir))
    #
    evio_dir = os.getenv('EVIO')
    evio_arch = platform + '-' + machine
    if evio_dir:
        evio_lib_dir2 = os.path.join(evio_dir,evio_arch,'lib')
        evio_inc_dir2 = os,path.join(evio_dir,evio_arch,'include')
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
    if coda_dir:
        evio_lib_dir3 = os.path.join(coda_dir,evio_arch,'lib')
        evio_inc_dir3 = os.path.join(coda_dir,evio_arch,'include')
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
    if th1 and tl1:
        env.Append(EVIO_LIB = evio_lib_dir)
        env.Append(EVIO_INC = evio_inc_dir)
    elif th2 and tl2:
        env.Append(EVIO_LIB = evio_lib_dir2)
        env.Append(EVIO_INC = evio_inc_dir2)
    elif th3 and tl3:
        env.Append(EVIO_LIB = evio_lib_dir3)
        env.Append(EVIO_INC = evio_inc_dir3)
    elif build_it:
        print ("No external EVIO environment configured !!!")
        print ("Using local installation ... ")
        evio_version = '4.4.6'
        evio_revision = 'evio-%s' % evio_version
        evio_tarfile = evio_revision + ".tar.gz"
        evio_local = os.path.join(env.Dir('.').abspath,'evio')
        evio_unpack_dir = os.path.join(evio_local,evio_revision)
        evio_libsrc =  os.path.join(evio_unpack_dir,"src","libsrc")
        if not os.path.exists(evio_local):
            os.makedirs(evio_local, mode=0o755)
        evio_tarpath = os.path.join(evio_local,evio_tarfile)

        if not os.path.isfile(os.path.join(evio_local,'evio.h')):
            # If needed, download EVIO archive
            if not os.path.exists(evio_tarpath):
                evio_url = 'https://github.com/JeffersonLab/hallac_evio/archive/%s' % evio_tarfile
                print('Dowloading EVIO tarball %s' % evio_url)
                urlhandle = urllib2.urlopen(evio_url)
                with open(evio_tarpath,'wb') as out_file:
                    shutil.copyfileobj( urlhandle, out_file )
                urlhandle.close()
            # Extract EVIO C-API sources (libsrc directory)
            print('Extracting EVIO tarball %s' % evio_tarpath)
            tar = tarfile.open(evio_tarpath,'r')
            tar_members = tar.getmembers()
            top_dir = tar_members[0].name.split('/')[0]
            to_extract = []
            for m in tar_members:
                if '/libsrc/' in m.name:
                    to_extract.append(m)
            tar.extractall( evio_local, to_extract )
            tar.close()
            os.rename( os.path.join(evio_local,top_dir), evio_unpack_dir )
            have_win32 = (env['PLATFORM']=='win32')
            for f in glob.glob(os.path.join(evio_libsrc,"*.[ch]")):
                if have_win32 or not 'msinttypes' in f:
                    shutil.copy2(f, evio_local)
            shutil.rmtree(evio_unpack_dir)
        # Build EVIO libsrc
        env.SConscript(os.path.join(evio_local,"SConscript.py"))

        env.Append(EVIO_LIB = evio_local)
        env.Append(EVIO_INC = evio_local)
        env.Replace(LOCAL_EVIO = 1)
    elif fail_if_missing:
        print('!!! Cannot find EVIO library.  Set EVIO_LIBDIR/EVIO_INCDIR, EVIO or CODA.')
        env.Exit(1)
    else:
        return

    print ("EVIO lib Directory = %s" % env.subst('$EVIO_LIB'))
    print ("EVIO include Directory = %s" % env.subst('$EVIO_INC'))

#end configure.py
