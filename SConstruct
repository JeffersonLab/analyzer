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

baseenv = Environment(ENV = os.environ,tools=["default","disttar"],toolpath='.')

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
        rootcint = env.subst('$ROOTCINT')
        print ("Doing rootcint call now ...")
        for f in source:
                headers += str(f) + " "
        command = rootcint + " -f %s -c -pthread -fPIC %s %s" % (dictname,cpppath,headers)
#	print ('RootCint Command = %s\n' % command)
        ok = os.system(command)
        return ok

baseenv.Append(ROOTCONFIG = 'root-config')
baseenv.Append(ROOTCINT = 'rootcint')
try:
        baseenv.AppendENVPath('PATH',baseenv['ENV']['ROOTSYS'] + '/bin')
except KeyError:
        pass    # ROOTSYS not defined

try:
        baseenv.ParseConfig('$ROOTCONFIG --cflags --libs')
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
evio_libdir = os.getenv('EVIO_LIBDIR')
evio_incdir = os.getenv('EVIO_INCDIR')
if evio_libdir is None or evio_incdir is None:
        print ("No external EVIO environment configured !!!")
        print ("EVIO_LIBDIR = %s" % evio_libdir)
        print ("EVIO_INCDIR = %s" % evio_incdir)
        print ("Using local installation ... ")
        evio_local = baseenv.subst('$HA_DIR')+'/evio'
        evio_command_dircreate = "mkdir -p %s" % (evio_local)
        os.system(evio_command_dircreate)
        evio_version = '4.4.6'
        uname = os.uname();
        platform = uname[0];
        machine = uname[4];
        evio_name = platform + '-' + machine
        print ("evio_name = %s" % evio_name)
        evio_local_lib = "%s/evio-%s/%s/lib" % (evio_local,evio_version,evio_name)
        evio_local_inc = "%s/evio-%s/%s/include" % (evio_local,evio_version,evio_name)
        evio_tarfile = "%s/evio-%s.tar.gz" % (evio_local,evio_version)

        ####### Check to see if scons -c has been called #########

        if baseenv.GetOption('clean'):
                subprocess.call(['echo', '!!!!!!!!!!!!!! EVIO Cleaning Process !!!!!!!!!!!! '])
                if not os.path.isdir(evio_local_lib):
                        if not os.path.exists(evio_tarfile):
                                evio_command_scons = "rm libevio*.*; cd %s; wget https://github.com/JeffersonLab/hallac_evio/archive/evio-%s.tar.gz; tar xvfz evio-%s.tar.gz; mv hallac_evio-evio-%s evio-%s; cd evio-%s/ ; scons install -c --prefix=." % (evio_local,evio_version,evio_version,evio_version,evio_version,evio_version)
                        else:
                                evio_command_scons = "rm libevio*.*; cd %s; tar xvfz evio-%s.tar.gz; mv hallac_evio-evio-%s evio-%s; cd evio-%s/ ; scons install -c --prefix=." % (evio_local,evio_version,evio_version,evio_version,evio_version)
                else:
                        evio_command_scons = "rm libevio*.*; cd %s; cd evio-%s/ ; scons install -c --prefix=." % (evio_local,evio_version)
                print ("evio_command_scons = %s" % evio_command_scons)
                os.system(evio_command_scons)
        else:
                if not os.path.isdir(evio_local_lib):
                        if not os.path.exists(evio_tarfile):
                                evio_command_scons = "cd %s; wget https://github.com/JeffersonLab/hallac_evio/archive/evio-%s.tar.gz; tar xvfz evio-%s.tar.gz; mv hallac_evio-evio-%s evio-%s; cd evio-%s/ ; scons install --prefix=." % (evio_local,evio_version,evio_version,evio_version,evio_version,evio_version)
                        else:
                                evio_command_scons = "cd %s; tar xvfz evio-%s.tar.gz; mv hallac_evio-evio-%s evio-%s; cd evio-%s/ ; scons install --prefix=." % (evio_local,evio_version,evio_version,evio_version,evio_version)
                else:
                        evio_command_scons = "cd %s; cd evio-%s/ ; scons install --prefix=." % (evio_local,evio_version)
                print ("evio_command_scons = %s" % evio_command_scons)
                os.system(evio_command_scons)
                evio_local_lib_files = "%s/*.*" % (evio_local_lib)
                evio_command_libcopy = "cp %s ." % (evio_local_lib_files)
                print ("evio_command_libcopy = %s" % evio_command_libcopy)
                os.system(evio_command_libcopy)

        baseenv.Append(EVIO_LIB = evio_local_lib)
        baseenv.Append(EVIO_INC = evio_local_inc)
else:
        # evio_command_scons = "cd %s; scons install --prefix=." % evio_instdir
        # os.system(evio_command_scons)
        baseenv.Append(EVIO_LIB = os.getenv('EVIO_LIBDIR'))
        baseenv.Append(EVIO_INC = os.getenv('EVIO_INCDIR'))
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
