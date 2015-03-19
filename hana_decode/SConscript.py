###### Hall A Software hana_decode SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013

import os
import re
import SCons.Util
Import('baseenv')

standalone = baseenv.subst('$STANDALONE')
print ('Compiling decoder executables:  STANDALONE = %s\n' % standalone)

standalonelist = Split("""
tstoo tstfadc tstf1tdc tstskel tstio tdecpr prfact epicsd tdecex
""")
# Still to come, perhaps, are (etclient, tstcoda) which should be compiled
# if the ONLINE_ET variable is set.  

list = Split("""
THaUsrstrutils.C THaCrateMap.C THaCodaData.C 
THaEpics.C THaFastBusWord.C THaCodaFile.C THaSlotData.C 
THaEvData.C THaCodaDecoder.C SimDecoder.C
CodaDecoder.C Module.C VmeModule.C FastbusModule.C
Lecroy1877Module.C Lecroy1881Module.C Lecroy1875Module.C
Fadc250Module.C GenScaler.C Scaler560.C Scaler1151.C
Scaler3800.C Scaler3801.C F1TDCModule.C SkeletonModule.C
""")
#evio.C
#swap_util.C swapped_intcpy.c

#baseenv.Append(LIBPATH=['$HA_DIR'])

proceed = "1" or "y" or "yes" or "Yes" or "Y"
if baseenv.subst('$STANDALONE')==proceed:
        for scalex in standalonelist:
                pname = scalex

		if scalex=='epicsd':
			main = 'epics_main.C'
		else:
                	main = scalex+'_main.C'

                if scalex=='tdecex':
			pname = baseenv.Program(target = pname, source = list+[main,'THaGenDetTest.C','THaDecDict.so'])
		else:	
			pname = baseenv.Program(target = pname, source = list+[main,'THaDecDict.so'])
			
                baseenv.Install('../bin',pname)
                baseenv.Alias('install',['../bin'])

sotarget = 'dc'

#dclib = baseenv.SharedLibrary(target=sotarget, source = list+['THaDecDict.so'],SHLIBPREFIX='../lib',SHLIBVERSION=['$VERSION'],LIBS=[''])
dclib = baseenv.SharedLibrary(target=sotarget, source = list+['THaDecDict.so'],SHLIBPREFIX='../lib',LIBS=[''])
print ('Decoder shared library = %s\n' % dclib)

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

Clean(dclib,cleantarget)
Clean(dclib,localmajorcleantarget)
Clean(dclib,localshortcleantarget)

#baseenv.Install('../lib',dclib)
#baseenv.Alias('install',['../lib'])
