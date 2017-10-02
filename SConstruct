#!/usr/bin/env python
###### Hall A Software Main SConstruct Build File #####
###### Author:	Edward Brash (brash@jlab.org) June 2013

import os
import sys
import platform
import SCons
import subprocess
import configure

EnsureSConsVersion(2,1,0)

baseenv = Environment(ENV = os.environ,tools=["default","disttar"],toolpath=['site_scons'])

####### Hall A Build Environment #############
baseenv.Append(MAIN_DIR = Dir('.').abspath)
baseenv.Append(HA_DIR = baseenv.subst('$MAIN_DIR'))
baseenv.Append(HA_SRC = baseenv.subst('$HA_DIR')+'/src ')
baseenv.Append(HA_DC = baseenv.subst('$HA_DIR')+'/hana_decode ')
baseenv.Append(MAJORVERSION = '1')
baseenv.Append(MINORVERSION = '6')
baseenv.Append(PATCH = '0')
baseenv.Append(SOVERSION = baseenv.subst('$MAJORVERSION')+'.'+baseenv.subst('$MINORVERSION'))
baseenv.Append(VERSION = baseenv.subst('$SOVERSION')+'.'+baseenv.subst('$PATCH'))
baseenv.Append(NAME = 'analyzer-'+baseenv.subst('$VERSION'))
baseenv.Append(EXTVERS = '-beta3')
baseenv.Append(HA_VERSION = baseenv.subst('$VERSION')+baseenv.subst('$EXTVERS'))
#print ("Main Directory = %s" % baseenv.subst('$HA_DIR'))
print ("Software Version = %s" % baseenv.subst('$VERSION'))
ivercode = 65536*int(float(baseenv.subst('$SOVERSION'))) + \
           256*int(10*(float(baseenv.subst('$SOVERSION')) - \
                       int(float(baseenv.subst('$SOVERSION'))))) + \
                       int(float(baseenv.subst('$PATCH')))
baseenv.Append(VERCODE = ivercode)
baseenv.Append(CPPPATH = ['$HA_SRC','$HA_DC'])

####### ROOT Definitions ####################
def rootcint(target,source,env):
        """Executes the ROOT dictionary generator over a list of headers."""
        dictname = target[0]
        headers = ""
        cpppath = env.subst('$_CCCOMCOM')
        ccflags = env.subst('$CCFLAGS')
#        print ("Doing rootcint call now ...")
        for f in source:
                headers += str(f) + " "
        command = "rootcint -f %s -c -pthread -fPIC %s %s" % (dictname,cpppath,headers)
#	print ('RootCint Command = %s\n' % command)
        ok = os.system(command)
        return ok

baseenv.Append(ROOTCONFIG = 'root-config')
try:
        baseenv.AppendENVPath('PATH',baseenv['ENV']['ROOTSYS'] + '/bin')
except KeyError:
        pass    # ROOTSYS not defined

try:
        baseenv.ParseConfig('$ROOTCONFIG --cflags --glibs')
        if sys.version_info >= (2, 7):
                cmd = baseenv['ROOTCONFIG'] + " --cxx"
                baseenv.Replace(CXX = subprocess.check_output(cmd, shell=True).rstrip())
                cmd = baseenv['ROOTCONFIG'] + " --version"
                baseenv.Replace(ROOTVERS = subprocess.check_output(cmd, shell=True).rstrip())
        else:
                baseenv.Replace(CXX = subprocess.Popen([baseenv['ROOTCONFIG'], '--cxx'],\
                                        stdout=subprocess.PIPE).communicate()[0].rstrip())
                baseenv.Replace(ROOTVERS = subprocess.Popen([baseenv['ROOTCONFIG'],\
                        '--version'],stdout=subprocess.PIPE).communicate()[0].rstrip())
        if platform.system() == 'Darwin':
                try:
                        baseenv.Replace(LINKFLAGS = baseenv['LINKFLAGS'].remove('-pthread'))
                except:
                        pass #  '-pthread' was not present in LINKFLAGS
except OSError:
        print('!!! Cannot find ROOT.  Check if root-config is in your PATH.')
        Exit(1)

bld = Builder(action=rootcint)
baseenv.Append(BUILDERS = {'RootCint': bld})

#
# evio environment
#
uname = os.uname();
platform = uname[0];
machine = uname[4];
if platform == 'Linux':
	sosuf = 'so'
if platform == 'Darwin':
	sosuf = 'dylib'
evio_header_file = 'evio.h'
evio_library_file = 'libevio.'+ sosuf
#
evio_inc_dir = os.getenv('EVIO_INCDIR')
evio_lib_dir = os.getenv('EVIO_LIBDIR')
th1 = FindFile(evio_header_file,evio_inc_dir)
tl1 = FindFile(evio_library_file,evio_lib_dir)
#print '%-12s' % ('%s:' % evio_header_file), FindFile(evio_header_file, evio_inc_dir)
#print '%-12s' % ('%s:' % evio_library_file), FindFile(evio_library_file, evio_lib_dir)
#
evio_dir = os.getenv('EVIO')
evio_arch = platform + '-' + machine
if evio_dir is not None:
	evio_lib_dir2 = evio_dir + '/' + evio_arch + '/lib'  
	evio_inc_dir2 = evio_dir + '/' + evio_arch + '/include' 
	th2 = FindFile(evio_header_file,evio_inc_dir2)
	tl2 = FindFile(evio_library_file,evio_lib_dir2)
else:
	evio_lib_dir2 = None
	evio_inc_dir2 = None
	th2 = None
	tl2 = None
#print '%-12s' % ('%s:' % evio_header_file), FindFile(evio_header_file, evio_inc_dir2)
#print '%-12s' % ('%s:' % evio_library_file), FindFile(evio_library_file, evio_lib_dir2)
#
coda_dir = os.getenv('CODA')
if coda_dir is not None:
	evio_lib_dir3 = coda_dir + '/' + evio_arch + '/lib'  
	evio_inc_dir3 = coda_dir + '/' + evio_arch + '/include'  
	th3 = FindFile(evio_header_file,evio_inc_dir3)
	tl3 = FindFile(evio_library_file,evio_lib_dir3)
else:
	evio_lib_dir3 = None
	evio_inc_dir3 = None
	th3 = None
	tl3 = None
#print '%-12s' % ('%s:' % evio_header_file), FindFile(evio_header_file, evio_inc_dir3)
#print '%-12s' % ('%s:' % evio_library_file), FindFile(evio_library_file, evio_lib_dir3)
#
if th1 is not None and tl1 is not None:
        baseenv.Append(EVIO_LIB = evio_lib_dir)
        baseenv.Append(EVIO_INC = evio_inc_dir)
else:
	if th2 is not None and tl2 is not None:
        	baseenv.Append(EVIO_LIB = evio_lib_dir2)
        	baseenv.Append(EVIO_INC = evio_inc_dir2)
	else:
		if th3 is not None and tl3 is not None:
        		baseenv.Append(EVIO_LIB = evio_lib_dir3)
        		baseenv.Append(EVIO_INC = evio_inc_dir3)
		else:
        		print ("No external EVIO environment configured !!!")
        		print ("Using local installation ... ")
        		evio_version = '4.4.6'
        		evio_local = baseenv.subst('$HA_DIR')+'/evio'
        		evio_local_lib = "%s/evio-%s/src/libsrc/.%s" % (evio_local,evio_version,evio_arch)
        		evio_local_inc = "%s/evio-%s/src/libsrc" % (evio_local,evio_version)
        		evio_tarfile = "%s/evio-%s.tar.gz" % (evio_local,evio_version)
        		evio_command_dircreate = "mkdir -p %s" % (evio_local)
        		os.system(evio_command_dircreate)

        		####### Check to see if scons -c has been called #########
        		if baseenv.GetOption('clean'):
                		subprocess.call(['echo', '!!!!!!!!!!!!!! EVIO Cleaning Process !!!!!!!!!!!! '])
				evio_command_cleanup = "rm -f libevio*.*; cd %s; rm -rf evio-%s" % (evio_local,evio_version)
                		print ("evio_command_cleanup = %s" % evio_command_cleanup)
                		os.system(evio_command_cleanup)
        		else:
                		if not os.path.isdir(evio_local_lib):
                        		if not os.path.exists(evio_tarfile):
                                		evio_command_download = "cd %s; curl -LO https://github.com/JeffersonLab/hallac_evio/archive/evio-%s.tar.gz" % (evio_local,evio_version)
                                		print ("evio_command_download = %s" % evio_command_download)
                                		os.system(evio_command_download)

                        		evio_command_unpack = "cd %s; tar xvfz evio-%s.tar.gz; mv hallac_evio-evio-%s evio-%s; patch -p0 < evio-4.4.6-scons-3.0.patch" % (evio_local,evio_version,evio_version,evio_version)
                        		print ("evio_command_unpack = %s" % evio_command_unpack)
                        		os.system(evio_command_unpack)

                		evio_command_scons = "cd %s/evio-%s; scons src/libsrc/.%s/" % (evio_local,evio_version,evio_arch)
                		print ("evio_command_scons = %s" % evio_command_scons)
                		os.system(evio_command_scons)
                		evio_local_lib_files = "%s/libevio.*" % (evio_local_lib)
                		evio_command_libcopy = "cp -vf %s ." % (evio_local_lib_files)
                		print ("evio_command_libcopy = %s" % evio_command_libcopy)
                		os.system(evio_command_libcopy)

        		baseenv.Append(EVIO_LIB = evio_local_lib)
        		baseenv.Append(EVIO_INC = evio_local_inc)

print ("EVIO lib Directory = %s" % baseenv.subst('$EVIO_LIB'))
print ("EVIO include Directory = %s" % baseenv.subst('$EVIO_INC'))
baseenv.Append(CPPPATH = ['$EVIO_INC'])
#
# end evio environment
#

######## cppcheck ###########################

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

proceed = "1" or "y" or "yes" or "Yes" or "Y"
if baseenv.subst('$CPPCHECK')==proceed:
        is_cppcheck = which('cppcheck')
        print ("Path to cppcheck is %s\n" % is_cppcheck)

        if(is_cppcheck == None):
                print('!!! cppcheck not found on this system.  Check if cppcheck is installed and in your PATH.')
                Exit(1)
        else:
                cppcheck_command = baseenv.Command('cppcheck_report.txt',[],"cppcheck --quiet --enable=all src/ hana_decode/ 2> $TARGET")
                print ("cppcheck_command = %s" % cppcheck_command)
                baseenv.AlwaysBuild(cppcheck_command)

####### build source distribution tarball #############

if baseenv.subst('$SRCDIST')==proceed:
        baseenv['DISTTAR_FORMAT']='gz'
        baseenv.Append(
                    DISTTAR_EXCLUDEEXTS=['.o','.os','.so','.a','.dll','.cache','.pyc','.cvsignore','.dblite','.log', '.gz', '.bz2', '.zip','.pcm','.supp','.patch','.txt','.dylib']
                    , DISTTAR_EXCLUDEDIRS=['.git','VDCsim','bin','scripts','.sconf_temp','tests','work','hana_scaler']
                    , DISTTAR_EXCLUDERES=[r'Dict\.C$', r'Dict\.h$',r'analyzer',r'\~$',r'\.so\.',r'\.#', r'\.dylib\.']
        )
        tar = baseenv.DistTar("dist/analyzer-"+baseenv.subst('$VERSION'),[baseenv.Dir('#')])
        print ("tarball target = %s" % tar)

###	tar_command = 'tar cvz -C .. -f ../' + baseenv.subst('$NAME') + '.tar.gz -X .exclude -V "JLab/Hall A C++ Analysis Software '+baseenv.subst('$VERSION') + ' `date -I`" ' + '../' + baseenv.subst('$NAME') + '/.exclude ' + '../' + baseenv.subst('$NAME') + '/Changelog ' + '../' + baseenv.subst('$NAME') + '/src ' + '../' + baseenv.subst('$NAME') + '/hana_decode ' + '../' + baseenv.subst('$NAME') + '/Makefile ' + '../' + baseenv.subst('$NAME') + '/*.py' + '../' + baseenv.subst('$NAME') + '/SConstruct'

######## Configure Section #######

if not (baseenv.GetOption('clean') or baseenv.GetOption('help')):

        configure.config(baseenv,ARGUMENTS)

        conf = Configure(baseenv)
        if not conf.CheckCXX():
                print('!!! Your compiler and/or environment is not correctly configured.')
                Exit(1)
        # if not conf.CheckFunc('printf'):
        #         print('!!! Your compiler and/or environment is not correctly configured.')
        #         Exit(1)
        if conf.CheckCXXHeader('sstream'):
                conf.env.Append(CPPDEFINES = 'HAS_SSTREAM')
        baseenv = conf.Finish()

Export('baseenv')

#print (baseenv.Dump())
#print ('CXXFLAGS = ', baseenv['CXXFLAGS'])
#print ('LINKFLAGS = ', baseenv['LINKFLAGS'])
#print ('SHLINKFLAGS = ', baseenv['SHLINKFLAGS'])

####### Start of main SConstruct ############

hallalib = 'HallA'
dclib = 'dc'
eviolib = 'evio'

baseenv.Append(LIBPATH=['$HA_DIR','$EVIO_LIB','$HA_SRC','$HA_DC'])
baseenv.Prepend(LIBS=[hallalib,dclib,eviolib])
baseenv.Replace(SOSUFFIX = baseenv.subst('$SHLIBSUFFIX'))
#baseenv.Replace(SHLIBSUFFIX = '.so')
baseenv.Append(SHLIBSUFFIX = '.'+baseenv.subst('$VERSION'))

SConscript(dirs = ['./','src/','hana_decode/'],name='SConscript.py',exports='baseenv')

#######  End of SConstruct #########

# Local Variables:
# mode: python
# End:
