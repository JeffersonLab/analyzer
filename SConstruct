## -*- mode: Python -*-

##### Hall A Software Main SConstruct Build File #####
###### Author:	Edward Brash (brash@jlab.org) June 2013

import os
import sys
import SCons
import subprocess
import configure

EnsureSConsVersion(2,1,0)

baseenv = Environment(ENV = os.environ)

####### Hall A Build Environment #############
baseenv.Append(MAIN_DIR = Dir('.').abspath)
baseenv.Append(HA_DIR = baseenv.subst('$MAIN_DIR'))
baseenv.Append(HA_SRC = baseenv.subst('$HA_DIR')+'/src ')
baseenv.Append(HA_DC = baseenv.subst('$HA_DIR')+'/hana_decode ')
baseenv.Append(HA_SCALER = baseenv.subst('$HA_DIR')+'/hana_scaler ')
baseenv.Append(MAJORVERSION = '1')
baseenv.Append(MINORVERSION = '5')
baseenv.Append(PATCH = '35')
baseenv.Append(SOVERSION = baseenv.subst('$MAJORVERSION')+'.'+baseenv.subst('$MINORVERSION'))
baseenv.Append(VERSION = baseenv.subst('$SOVERSION')+'.'+baseenv.subst('$PATCH'))
baseenv.Append(EXTVERS = '')
baseenv.Append(HA_VERSION = baseenv.subst('$VERSION')+baseenv.subst('$EXTVERS'))
#print "Main Directory = %s" % baseenv.subst('$HA_DIR')
print "Software Version = %s" % baseenv.subst('$VERSION')
ivercode = 65536*int(float(baseenv.subst('$SOVERSION'))) + \
           256*int(10*(float(baseenv.subst('$SOVERSION')) - \
                       int(float(baseenv.subst('$SOVERSION'))))) + \
                       int(float(baseenv.subst('$PATCH')))
baseenv.Append(VERCODE = ivercode)
baseenv.Append(CPPPATH = ['$HA_SRC','$HA_DC','$HA_SCALER'])

####### ROOT Definitions ####################
def rootcint(target,source,env):
        """Executes the ROOT dictionary generator over a list of headers."""
        dictname = target[0]
        headers = ""
        cpppath = env.subst('$_CCCOMCOM')
        ccflags = env.subst('$CCFLAGS')
        rootcint = env.subst('$ROOTCINT')
#        print "Doing rootcint call now ..."
        for f in source:
                headers += str(f) + " "
        command = rootcint + " -f %s -c -pthread -fPIC %s %s" % (dictname,cpppath,headers)
#        print ('RootCint Command = %s' % command)
        ok = os.system(command)
        return ok

baseenv.Append(ROOTCONFIG = 'root-config')
baseenv.Append(ROOTCINT = 'rootcint')
try:
        baseenv.AppendENVPath('PATH',baseenv['ENV']['ROOTSYS'] + '/bin')
except KeyError:
        pass    # ROOTSYS not defined

try:
        baseenv.ParseConfig('$ROOTCONFIG --cflags')
        baseenv.ParseConfig('$ROOTCONFIG --libs')
        if sys.version_info >= (2, 7):
                cmd = baseenv['ROOTCONFIG'] + " --cxx"
                baseenv.Replace(CXX = subprocess.check_output(cmd, shell=True).rstrip())
                cmd = baseenv['ROOTCONFIG'] + " --version"
                baseenv.Replace(ROOTVERS = subprocess.check_output(cmd, shell=True).rstrip())
        else:
                baseenv.Replace(CXX = subprocess.Popen([baseenv['ROOTCONFIG'], '--cxx'], \
                                        stdout=subprocess.PIPE).communicate()[0].rstrip())
                baseenv.Replace(ROOTVERS = subprocess.Popen([baseenv['ROOTCONFIG'], '--version'], \
                                        stdout=subprocess.PIPE).communicate()[0].rstrip())
        if baseenv.subst('$CXX') == 'clang++':
                try:
                        baseenv.Replace(LINKFLAGS = baseenv['LINKFLAGS'].remove('-pthread'))
                except:
                        pass #  '-pthread' was not present in LINKFLAGS

except OSError:
        print('!!! Cannot find ROOT.  Check if root-config is in your PATH.')
        Exit(1)

bld = Builder(action=rootcint)
baseenv.Append(BUILDERS = {'RootCint': bld})

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

#print baseenv.Dump()
#print 'CXXFLAGS = ', baseenv['CXXFLAGS']
#print 'LINKFLAGS = ', baseenv['LINKFLAGS']

####### Start of main SConstruct ############

hallalib = 'HallA'
dclib = 'dc'
scalerlib = 'scaler'

baseenv.Append(LIBPATH=['$HA_DIR','$HA_SRC','$HA_DC','$HA_SCALER'])
#baseenv.Append(LIBPATH=['$HA_LIB'])
baseenv.Prepend(LIBS=[hallalib,dclib,scalerlib])
baseenv.Replace(SHLIBSUFFIX = '.so')
baseenv.Append(SHLIBSUFFIX = '.'+baseenv.subst('$VERSION'))

SConscript(dirs = ['./','src/','hana_decode/','hana_scaler'],name='SConscript.py',exports='baseenv')

#######  End of SConstruct #########
