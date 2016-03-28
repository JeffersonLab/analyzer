#!/usr/bin/env python
###### Hall A Software Main SConstruct Build File #####
###### Author:	Edward Brash (brash@jlab.org) June 2013

import os
import sys
import platform
import commands
import SCons
import time
import subprocess

def rootcint(target,source,env):
	"""Executes the ROOT dictionary generator over a list of headers."""
	dictname = target[0]
	headers = ""
	cpppath = env.subst('$_CCCOMCOM')
	ccflags = env.subst('$CCFLAGS')
	rootcint = env.subst('$ROOTCINT')
	print "Doing rootcint call now ..."
	for f in source:
		headers += str(f) + " "
	command = rootcint + " -f %s -c -pthread -fPIC %s %s" % (dictname,cpppath,headers)
	print ('RootCint Command = %s\n' % command)
	ok = os.system(command)
	return ok

baseenv = Environment(ENV = os.environ,tools=["default","disttar"],toolpath='.')
#dict = baseenv.Dictionary()
#keys = dict.keys()
#keys.sort()
#for key in keys:
#	print "Construction variable = '%s', value = '%s'" % (key, dict[key])

####### Check SCons version ##################
#print('!!! You should be using the local version of SCons, invoked with:')
#print('!!! ./scons/scons.py')
print('!!! Building the Hall A analyzer and libraries with SCons requires')
print('!!! SCons version 2.1.0 or newer.')
EnsureSConsVersion(2,1,0)

####### Hall A Build Environment #############
#
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
baseenv.Append(EXTVERS = '-alpha4')
baseenv.Append(HA_VERSION = baseenv.subst('$VERSION')+baseenv.subst('$EXTVERS'))
print "Main Directory = %s" % baseenv.subst('$HA_DIR')
print "Software Version = %s" % baseenv.subst('$VERSION')
ivercode = 65536*int(float(baseenv.subst('$SOVERSION')))+ 256*int(10*(float(baseenv.subst('$SOVERSION'))-int(float(baseenv.subst('$SOVERSION')))))+ int(float(baseenv.subst('$PATCH')))
baseenv.Append(VERCODE = ivercode)
#
# evio environment 
#
evio_libdir = os.getenv('EVIO_LIBDIR')
evio_incdir = os.getenv('EVIO_INCDIR')
if evio_libdir is None or evio_incdir is None:
	print "No external EVIO environment configured !!!"
	print "EVIO_LIBDIR = %s" % evio_libdir
	print "EVIO_INCDIR = %s" % evio_incdir
	print "Using local installation ... "
	evio_local = baseenv.subst('$HA_DIR')+'/evio'
	evio_command_dircreate = "mkdir -p %s" % (evio_local)
	os.system(evio_command_dircreate)
	evio_version = '4.4.6'
	uname = os.uname();
	platform = uname[0];
	machine = uname[4];
	evio_name = platform + '-' + machine
	print "evio_name = %s" % evio_name	
	evio_local_lib = "%s/evio-%s/%s/lib" % (evio_local,evio_version,evio_name) 
	evio_local_inc = "%s/evio-%s/%s/include" % (evio_local,evio_version,evio_name)
	evio_tarfile = "%s/evio-%s.tgz" % (evio_local,evio_version)

	####### Check to see if scons -c has been called #########

	if baseenv.GetOption('clean'):
    		subprocess.call(['echo', '!!!!!!!!!!!!!! EVIO Cleaning Process !!!!!!!!!!!! '])
		if not os.path.isdir(evio_local_lib):
			if not os.path.exists(evio_tarfile):
				evio_command_scons = "rm libevio*.*; cd %s; wget --no-check-certificate https://coda.jlab.org/drupal/system/files/evio-%s.tgz; tar xvfz evio-%s.tgz; cd evio-%s/ ; scons install -c --prefix=." % (evio_local,evio_version,evio_version,evio_version)
			else:
				evio_command_scons = "rm libevio*.*; cd %s; tar xvfz evio-%s.tgz; cd evio-%s/ ; scons install -c --prefix=." % (evio_local,evio_version,evio_version)
		else:
			evio_command_scons = "rm libevio*.*; cd %s; cd evio-%s/ ; scons install -c --prefix=." % (evio_local,evio_version)
		print "evio_command_scons = %s" % evio_command_scons
		os.system(evio_command_scons)
	else:
		if not os.path.isdir(evio_local_lib):
			if not os.path.exists(evio_tarfile):
				evio_command_scons = "cd %s; wget --no-check-certificate https://coda.jlab.org/drupal/system/files/evio-%s.tgz; tar xvfz evio-%s.tgz; cd evio-%s/ ; scons install --prefix=." % (evio_local,evio_version,evio_version,evio_version)
			else:
				evio_command_scons = "cd %s; tar xvfz evio-%s.tgz; cd evio-%s/ ; scons install --prefix=." % (evio_local,evio_version,evio_version)
		else:
			evio_command_scons = "cd %s; cd evio-%s/ ; scons install --prefix=." % (evio_local,evio_version)
		print "evio_command_scons = %s" % evio_command_scons
		os.system(evio_command_scons)
		evio_local_lib_files = "%s/*.*" % (evio_local_lib)
		evio_command_libcopy = "cp %s ." % (evio_local_lib_files)
		print "evio_command_libcopy = %s" % evio_command_libcopy
		os.system(evio_command_libcopy)

	baseenv.Append(EVIO_LIB = evio_local_lib)
	baseenv.Append(EVIO_INC = evio_local_inc)
else:
	# evio_command_scons = "cd %s; scons install --prefix=." % evio_instdir
	# os.system(evio_command_scons)
	baseenv.Append(EVIO_LIB = os.getenv('EVIO_LIBDIR'))
	baseenv.Append(EVIO_INC = os.getenv('EVIO_INCDIR'))
print "EVIO lib Directory = %s" % baseenv.subst('$EVIO_LIB')
print "EVIO include Directory = %s" % baseenv.subst('$EVIO_INC')
baseenv.Append(CPPPATH = ['$EVIO_INC'])
#
# end evio environment
#
baseenv.Append(CPPPATH = ['$HA_SRC','$HA_DC'])

####### ROOT Definitions ####################
baseenv.Append(ROOTCONFIG = 'root-config')
baseenv.Append(ROOTCINT = 'rootcint')

try:
	baseenv.ParseConfig('$ROOTCONFIG --cflags')
	baseenv.ParseConfig('$ROOTCONFIG --libs')
        if sys.version_info >= (2, 7):
                cmd = baseenv['ROOTCONFIG'] + " --cxx"
                baseenv.Replace(CXX = subprocess.check_output(cmd, shell=True).rstrip())
        else:
                baseenv.Replace(CXX = subprocess.Popen([baseenv['ROOTCONFIG'], '--cxx'], stdout=subprocess.PIPE).communicate()[0].rstrip())
except OSError:
	try:
		baseenv.Replace(ROOTCONFIG = baseenv['ENV']['ROOTSYS'] + '/bin/root-config')
		baseenv.Replace(ROOTCINT = baseenv['ENV']['ROOTSYS'] + '/bin/rootcint')
		baseenv.ParseConfig('$ROOTCONFIG --cflags')
		baseenv.ParseConfig('$ROOTCONFIG --libs')
                if sys.version_info >= (2, 7):
                      cmd = baseenv['ROOTCONFIG'] + " --cxx"
                      baseenv.Replace(CXX = subprocess.check_output(cmd, shell=True).rstrip())
                else:
                      baseenv.Replace(CXX = subprocess.Popen([baseenv['ROOTCONFIG'], '--cxx'], stdout=subprocess.PIPE).communicate()[0].rstrip())
	except KeyError:
       		print('!!! Cannot find ROOT.  Check if root-config is in your PATH.')
		Exit(1)

bld = Builder(action=rootcint)
baseenv.Append(BUILDERS = {'RootCint': bld})

######## Configure Section #######

import configure
configure.config(baseenv,ARGUMENTS)

Export('baseenv')

conf = Configure(baseenv)

if not conf.CheckCXX():
	print('!!! Your compiler and/or environment is not correctly configured.')
	Exit(0)

#if not conf.CheckFunc('printf'):
if not conf.CheckCC():
        print('!!! Your compiler and/or environment is not correctly configured.')
        Exit(0)

baseenv = conf.Finish()

######## cppcheck ###########################

def which(program):
	import os
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
	print "Path to cppcheck is %s\n" % is_cppcheck

	if(is_cppcheck == None):
		print('!!! cppcheck not found on this system.  Check if cppcheck is installed and in your PATH.')
		Exit(1)
	else:
		cppcheck_command = baseenv.Command('cppcheck_report.txt',[],"cppcheck --quiet --enable=all src/ hana_decode/ 2> $TARGET")
		print "cppcheck_command = %s" % cppcheck_command
		baseenv.AlwaysBuild(cppcheck_command)

####### build source distribution tarball #############

if baseenv.subst('$SRCDIST')==proceed:
	baseenv['DISTTAR_FORMAT']='gz'
	baseenv.Append(
		    DISTTAR_EXCLUDEEXTS=['.o','.os','.so','.a','.dll','.cache','.pyc','.cvsignore','.dblite','.log', '.gz', '.bz2', '.zip']
		    , DISTTAR_EXCLUDEDIRS=['.git','Calib','docs', 'SDK','contrib','VDCsim','bin','examples','scripts','DB','.sconf_temp']
		    , DISTTAR_EXCLUDERES=[r'Dict\.C$', r'Dict\.h$',r'analyzer',r'\~$',r'\.so\.']
	)
	tar = baseenv.DistTar("dist/analyzer-"+baseenv.subst('$VERSION'),[baseenv.Dir('#')])
	print "tarball target = %s" % tar

###	tar_command = 'tar cvz -C .. -f ../' + baseenv.subst('$NAME') + '.tar.gz -X .exclude -V "JLab/Hall A C++ Analysis Software '+baseenv.subst('$VERSION') + ' `date -I`" ' + '../' + baseenv.subst('$NAME') + '/.exclude ' + '../' + baseenv.subst('$NAME') + '/Changelog ' + '../' + baseenv.subst('$NAME') + '/src ' + '../' + baseenv.subst('$NAME') + '/hana_decode ' + '../' + baseenv.subst('$NAME') + '/Makefile ' + '../' + baseenv.subst('$NAME') + '/*.py' + '../' + baseenv.subst('$NAME') + '/SConstruct'

####### Start of main SConstruct ############

hallalib = 'HallA'
dclib = 'dc'
eviolib = 'evio'

baseenv.Append(LIBPATH=['$HA_DIR','$EVIO_LIB','$HA_SRC','$HA_DC'])
baseenv.Prepend(LIBS=[hallalib,dclib,eviolib])
baseenv.Replace(SHLIBSUFFIX = '.so')
baseenv.Append(SHLIBSUFFIX = '.'+baseenv.subst('$VERSION'))

SConscript(dirs = ['./','src/','hana_decode/'],name='SConscript.py',exports='baseenv')

#######  End of SConstruct #########

# Local Variables:
# mode: python
# End:
