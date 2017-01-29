###### Hall A Software hana_decode SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013

import os
import re
import SCons.Util
Import('baseenv')

standalone = baseenv.subst('$STANDALONE')
#print ('Compiling decoder executables:  STANDALONE = %s' % standalone)

standalonelist = Split("""
tstio tdecpr prfact epicsd tdecex 
""")
# Still to come, perhaps, are (etclient, tstcoda) which should be compiled
# if the ONLINE_ET variable is set.  

list = Split("""
THaUsrstrutils.C THaCrateMap.C THaCodaData.C 
THaEpics.C THaFastBusWord.C THaCodaFile.C THaSlotData.C 
THaEvData.C evio.C THaCodaDecoder.C SimDecoder.C
swap_util.C swapped_intcpy.c
""")

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
			pname = baseenv.Program(target = pname, source = list+[main,'THaGenDetTest.C','THaDecDict.os'])
		else:	
			pname = baseenv.Program(target = pname, source = list+[main,'THaDecDict.os'])
			
                baseenv.Install('../bin',pname)
                baseenv.Alias('install',['../bin'])

sotarget = 'dc'

dclib = baseenv.SharedLibrary(target=sotarget, source = list+['THaDecDict.os'],SHLIBPREFIX='../lib',LIBS=[''])
#print ('Decoder shared library = %s' % dclib)

linkbase = baseenv.subst('$SHLIBPREFIX')+sotarget

cleantarget = linkbase+'.so.'+baseenv.subst('$VERSION')
majorcleantarget = linkbase+'.so'
localmajorcleantarget = '../'+linkbase+'.so'
shortcleantarget = linkbase+'.so.'+baseenv.subst('$SOVERSION')
localshortcleantarget = '../'+linkbase+'.so.'+baseenv.subst('$SOVERSION')

#print ('cleantarget = %s' % cleantarget)
#print ('majorcleantarget = %s' % majorcleantarget)
#print ('shortcleantarget = %s' % shortcleantarget)
try:
	os.symlink(cleantarget,localshortcleantarget)
	os.symlink(shortcleantarget,localmajorcleantarget)
except:	
        pass
        #	print " Continuing ... "

Clean(dclib,cleantarget)
Clean(dclib,localmajorcleantarget)
Clean(dclib,localshortcleantarget)

#baseenv.Install('../lib',dclib)
#baseenv.Alias('install',['../lib'])
