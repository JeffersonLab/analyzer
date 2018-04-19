###### Hall A SDK Main SConscript File #####
###### Author:	Edward Brash (brash@jlab.org) May 2017

import re

Import ('baseenv')

######## ROOT Dictionaries #########

rootuserdict = baseenv.subst('$MAIN_DIR')+'/RootUserDict.cxx'
userheaders = Split("""
UserApparatus.h    UserDetector.h     UserEvtHandler.h   UserModule.h       
UserScintillator.h SkeletonModule.h   User_LinkDef.h
""")
baseenv.RootCint(rootuserdict,userheaders)
baseenv.Clean(rootuserdict,re.sub(r'\.cxx\Z','_rdict.pcm',rootuserdict))

#######  Start of main SConscript ###########

list = Split("""
UserApparatus.cxx    UserModule.cxx
UserDetector.cxx     UserEvtHandler.cxx   UserScintillator.cxx SkeletonModule.cxx
""")

sotarget = 'User'
srclib = baseenv.SharedLibrary(target = sotarget, source = list+[rootuserdict],\
                               LIBS = [''], LIBPATH = [''])
