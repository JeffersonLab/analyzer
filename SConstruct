#!/usr/bin/env python
###### Hall A Software Main SConstruct Build File #####
###### Author:	Edward Brash (brash@jlab.org) June 2013

import os
import SCons
import configure
from rootcint import rootcint

EnsureSConsVersion(2,1,0)

baseenv = Environment(ENV = os.environ,tools=["default","disttar"],toolpath=['site_scons'])

####### Hall A Build Environment #############
baseenv.Append(MAIN_DIR = Dir('.').abspath)
baseenv.Append(HA_DIR = baseenv.subst('$MAIN_DIR'))
baseenv.Append(HA_SRC = baseenv.subst('$HA_DIR')+'/src ')
baseenv.Append(HA_DC = baseenv.subst('$HA_DIR')+'/hana_decode ')
baseenv.Append(MAJORVERSION = '1')
baseenv.Append(MINORVERSION = '7')
baseenv.Append(PATCH = '0')
baseenv.Append(SOVERSION = baseenv.subst('$MAJORVERSION')+'.'+baseenv.subst('$MINORVERSION'))
baseenv.Append(VERSION = baseenv.subst('$SOVERSION')+'.'+baseenv.subst('$PATCH'))
baseenv.Append(NAME = 'analyzer-'+baseenv.subst('$VERSION'))
baseenv.Append(EXTVERS = '-devel')
baseenv.Append(HA_VERSION = baseenv.subst('$VERSION')+baseenv.subst('$EXTVERS'))
#print ("Main Directory = %s" % baseenv.subst('$HA_DIR'))
print ("Software Version = %s" % baseenv.subst('$VERSION'))
ivercode = 65536*int(float(baseenv.subst('$SOVERSION'))) + \
           256*int(10*(float(baseenv.subst('$SOVERSION')) - \
                       int(float(baseenv.subst('$SOVERSION'))))) + \
                       int(float(baseenv.subst('$PATCH')))
baseenv.Append(VERCODE = ivercode)
baseenv.Append(CPPPATH = ['$HA_SRC','$HA_DC'])

configure.FindROOT(baseenv)
configure.FindEVIO(baseenv)

bld = Builder(action=rootcint)
baseenv.Append(BUILDERS = {'RootCint': bld})

######## cppcheck ###########################

proceed = "1" or "y" or "yes" or "Yes" or "Y"
if baseenv.subst('$CPPCHECK')==proceed:
    is_cppcheck = configure.which('cppcheck')
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
        , DISTTAR_EXCLUDERES=[r'Dict\.cxx$', r'Dict\.h$',r'analyzer',r'\~$',r'\.so\.',r'\.#', r'\.dylib\.']
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
       #Exit(1)
       # if not conf.CheckFunc('printf'):
       #         print('!!! Your compiler and/or environment is not correctly configured.')
       #         Exit(1)
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
