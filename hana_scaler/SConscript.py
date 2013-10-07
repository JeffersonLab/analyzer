###### Hall A Software hana_scaler SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013

import os
import re
import SCons.Util
Import('baseenv')

standalone = baseenv.subst('$STANDALONE')
print ('Compiling scaler executables:  STANDALONE = %s\n' % standalone)

standalonelist = Split("""
tscalfile tscalasy
tscalhist tscalonl tscalbad
tscalntup tscalbbite tscaldtime
tscalevt tscalring tscalfbk
tscalroc11 tscalroc23
""")

list = Split("""
THaScaler.C 
THaScalerDB.C
""")

#baseenv.Append(LIBPATH=['$HA_DIR'])

proceed = "1" or "y" or "yes" or "Yes" or "Y"
if baseenv.subst('$STANDALONE')==proceed:
	for scalex in standalonelist:
		pname = scalex
		main = scalex+'_main.C'
		pname = baseenv.Program(target = pname, source = list+[main,'THaScalDict.so'])
		baseenv.Install('../bin',pname)
		baseenv.Alias('install',['../bin'])

sotarget = 'scaler'

scalerlib = baseenv.SharedLibrary(target=sotarget, source = list+['THaScalDict.so'],SHLIBPREFIX='../lib',SHLIBVERSION=['$VERSION'],LIBS=[''])
print ('Scaler shared library = %s\n' % scalerlib)
#baseenv.Install('../lib',scalerlib)
#baseenv.Alias('install',['../lib'])
