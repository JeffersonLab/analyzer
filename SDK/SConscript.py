###### Hall A SDK Main SConscript File #####
###### Author:	Edward Brash (brash@jlab.org) May 2017

import os
import re
import SCons.Util
Import ('baseenv')

######## ROOT Dictionaries #########

rootuserdict = baseenv.subst('$MAIN_DIR')+'/RootUserDict.C'
rootuserobj = baseenv.subst('$MAIN_DIR')+'/RootUserDict.so'
userheaders = Split("""
UserApparatus.h    UserDetector.h     UserEvtHandler.h   UserModule.h       
UserScintillator.h SkeletonModule.h   User_LinkDef.h
""")
baseenv.RootCint(rootuserdict,userheaders)
baseenv.SharedObject(target = rootuserobj, source = rootuserdict)

#######  Start of main SConscript ###########

list = Split("""
UserApparatus.cxx    UserModule.cxx
UserDetector.cxx     UserEvtHandler.cxx   UserScintillator.cxx SkeletonModule.cxx
""")

sotarget = 'User'
srclib = baseenv.SharedLibrary(target = sotarget, source = list+['RootUserDict.so'],SHLIBPREFIX='lib',LIBS=[''])
print ('Source shared library = %s\n' % srclib)

linkbase = baseenv.subst('$SHLIBPREFIX')+sotarget

cleantarget = linkbase+'.so'

print ('cleantarget = %s\n' % cleantarget)

Clean(srclib,cleantarget)
