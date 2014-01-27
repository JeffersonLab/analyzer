###### Hall A Software hana_decode SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013

import os
import re
import SCons.Util
Import('baseenv')

standalone = baseenv.subst('$STANDALONE')
print ('Compiling object-oriented decoder executables:  STANDALONE = %s\n' % standalone)

standalonelist = Split("""
tstoy
""")
# Still to come, perhaps, are (etclient, tstcoda) which should be compiled
# if the ONLINE_ET variable is set.  

list = Split("""
THaUsrstrutils.C THaCrateMap.C THaCodaData.C 
THaEpics.C THaFastBusWord.C THaCodaFile.C THaSlotData.C 
THaEvData.C THaCodaDecoder.C
ToyCodaDecoder.C ToyEvtTypeHandler.C
ToyPhysicsEvtHandler.C ToyScalerEvtHandler.C
ToyModule.C ToyModuleX.C ToyFastbusModule.C
""")
#evio.C
#swap_util.C swapped_intcpy.c

#baseenv.Append(LIBPATH=['$HA_DIR'])

proceed = "1" or "y" or "yes" or "Yes" or "Y"
if baseenv.subst('$STANDALONE')==proceed:
        for scalex in standalonelist:
                pname = scalex

		if scalex=='tstoy':
			main = scalex+'_main.C'
		else:
			main = scalex+'_main.C'

                if scalex=='tstoy':
			pname = baseenv.Program(target = pname, source = list+[main,'THaOODecDict.so'])
		else:	
			pname = baseenv.Program(target = pname, source = list+[main,'THaOODecDict.so'])
			
                baseenv.Install('../bin',pname)
                baseenv.Alias('install',['../bin'])

sotarget = 'oodc'

#dclib = baseenv.SharedLibrary(target=sotarget, source = list+['THaDecDict.so'],SHLIBPREFIX='../lib',SHLIBVERSION=['$VERSION'],LIBS=[''])
oodclib = baseenv.SharedLibrary(target=sotarget, source = list+['THaOODecDict.so'],SHLIBPREFIX='../lib',LIBS=[''])
print ('Decoder shared library = %s\n' % oodclib)

linkbase = baseenv.subst('$SHLIBPREFIX')+sotarget

cleantarget = linkbase+'.so.'+baseenv.subst('$VERSION')
majorcleantarget = linkbase+'.so'
localmajorcleantarget = '../'+linkbase+'.so'
shortcleantarget = linkbase+'.so.'+baseenv.subst('$SOVERSION')
localshortcleantarget = '../'+linkbase+'.so.'+baseenv.subst('$SOVERSION')

print ('cleantarget = %s\n' % cleantarget)
print ('majorcleantarget = %s\n' % majorcleantarget)
print ('shortcleantarget = %s\n' % shortcleantarget)
try:
	os.symlink(cleantarget,localshortcleantarget)
	os.symlink(shortcleantarget,localmajorcleantarget)
except:	
	print " Continuing ... "

Clean(oodclib,cleantarget)
Clean(oodclib,localmajorcleantarget)
Clean(oodclib,localshortcleantarget)

#baseenv.Install('../lib',dclib)
#baseenv.Alias('install',['../lib'])