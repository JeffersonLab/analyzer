###### Hall A Software hana_decode SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013

import os
import re
import SCons.Util
Import('baseenv')

standalone = baseenv.subst('$STANDALONE')
#print ('Compiling decoder executables:  STANDALONE = %s\n' % standalone)

standalonelist = Split("""
tstoo tstfadc tstf1tdc tstio tdecpr prfact epicsd tdecex tst1190
""")
# Still to come, perhaps, are (etclient, tstcoda) which should be compiled
# if the ONLINE_ET variable is set.

list = Split("""
Caen1190Module.cxx
Caen775Module.cxx
Caen792Module.cxx
CodaDecoder.cxx
F1TDCModule.cxx
Fadc250Module.cxx
FastbusModule.cxx
GenScaler.cxx
Lecroy1875Module.cxx
Lecroy1877Module.cxx
Lecroy1881Module.cxx
Module.cxx
PipeliningModule.cxx
Scaler1151.cxx
Scaler3800.cxx
Scaler3801.cxx
Scaler560.cxx
SimDecoder.cxx
THaCodaData.cxx
THaCodaFile.cxx
THaCrateMap.cxx
THaEpics.cxx
THaEvData.cxx
THaSlotData.cxx
THaUsrstrutils.cxx
VmeModule.cxx
""")

# Requires SCons >= 2.3.5 for "exclude" keyword
#list = Glob('*.cxx',exclude=['*_main.cxx','*_onl.cxx','calc_thresh.cxx',
#                           'THaEtClient.cxx','THaGenDetTest.cxx'])

#baseenv.Append(LIBPATH=['$HA_DIR'])

proceed = "1" or "y" or "yes" or "Yes" or "Y"
if baseenv.subst('$STANDALONE')==proceed or baseenv.GetOption('clean'):
    for scalex in standalonelist:
        pname = scalex

        if scalex=='epicsd':
            main = 'epics_main.cxx'
        else:
            main = scalex+'_main.cxx'

        if scalex=='tdecex':
            pname = baseenv.Program(target = pname, source = [main,'THaGenDetTest.cxx'])
        else:
            pname = baseenv.Program(target = pname, source = [main])

        baseenv.Install('../bin',pname)
        baseenv.Alias('install',['../bin'])

sotarget = 'dc'

dclib = baseenv.SharedLibrary(target = sotarget,\
            source = list+[baseenv.subst('$MAIN_DIR')+'/THaDecDict.cxx'],\
            SHLIBPREFIX = baseenv.subst('$MAIN_DIR')+'/lib',\
            LIBS = [''], LIBPATH = [''])
#print ('Decoder shared library = %s\n' % dclib)

linkbase = baseenv.subst('$SHLIBPREFIX')+sotarget

cleantarget = linkbase+baseenv.subst('$SHLIBSUFFIX')
majorcleantarget = linkbase+baseenv.subst('$SOSUFFIX')
localmajorcleantarget = '../'+linkbase+baseenv.subst('$SOSUFFIX')
shortcleantarget = linkbase+baseenv.subst('$SOSUFFIX')+'.'+baseenv.subst('$SOVERSION')
localshortcleantarget = '../'+linkbase+baseenv.subst('$SOSUFFIX')+'.'+\
                        baseenv.subst('$SOVERSION')

#print ('cleantarget = %s\n' % cleantarget)
#print ('majorcleantarget = %s\n' % majorcleantarget)
#print ('shortcleantarget = %s\n' % shortcleantarget)
try:
    os.symlink(cleantarget,localshortcleantarget)
    os.symlink(shortcleantarget,localmajorcleantarget)
except:
    pass

Clean(dclib,cleantarget)
Clean(dclib,localmajorcleantarget)
Clean(dclib,localshortcleantarget)

#baseenv.Install('../lib',dclib)
#baseenv.Alias('install',['../lib'])
