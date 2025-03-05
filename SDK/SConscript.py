###### Hall A SDK Main SConscript File #####
###### Author:	Edward Brash (brash@jlab.org) May 2017

import os
import re
import SCons.Util
Import ('baseenv')

######## ROOT Dictionaries #########

rootuserdict = baseenv.subst('$MAIN_DIR')+'/SBSDict.C'
rootuserobj = baseenv.subst('$MAIN_DIR')+'/SBSDict.so'
userheaders = Split("""
FbusDetector.h FbusApparatus.h
HcalDetector.h HcalApparatus.h
CdetDetector.h CdetApparatus.h
SBS_LinkDef.h
""")
baseenv.RootCint(rootuserdict,userheaders)
baseenv.SharedObject(target = rootuserobj, source = rootuserdict)

#######  Start of main SConscript ###########

list = Split("""
FbusDetector.cxx FbusApparatus.cxx
HcalDetector.cxx HcalApparatus.cxx
CdetDetector.cxx CdetApparatus.cxx
""")

sotarget = 'SBS'
srclib = baseenv.SharedLibrary(target = sotarget, source = list+['SBSDict.so'],SHLIBPREFIX='lib',LIBS=[''])
print ('Source shared library = %s\n' % srclib)

linkbase = baseenv.subst('$SHLIBPREFIX')+sotarget

cleantarget = linkbase+'.so'

print ('cleantarget = %s\n' % cleantarget)

Clean(srclib,cleantarget)
