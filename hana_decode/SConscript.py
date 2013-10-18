###### Hall A Software hana_decode SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013

import os
import re
import SCons.Util
Import('baseenv')

standalone = baseenv.subst('$STANDALONE')
print ('Compiling decoder executables:  STANDALONE = %s\n' % standalone)

standalonelist = Split("""
tstio tdecpr prfact epicsd tdecex 
""")
# Still to come, perhaps, are (etclient, tstcoda) which should be compiled
# if the ONLINE_ET variable is set.  

list = Split("""
THaUsrstrutils.C THaCrateMap.C THaCodaData.C 
THaEpics.C THaFastBusWord.C THaCodaFile.C THaSlotData.C 
THaEvData.C THaCodaDecoder.C
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

dclib = baseenv.SharedLibrary(target=sotarget, source = list+['THaDecDict.so'],SHLIBPREFIX='../lib',SHLIBVERSION=['$VERSION'],LIBS=[''])
print ('Decoder shared library = %s\n' % dclib)
#baseenv.Install('../lib',dclib)
#baseenv.Alias('install',['../lib'])
