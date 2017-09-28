###### Hall A SDK Main SConscript File #####
###### Author:	Edward Brash (brash@jlab.org) May 2017

Import ('baseenv')

######## ROOT Dictionaries #########

rootuserdict = baseenv.subst('$MAIN_DIR')+'/RootUserDict.C'
userheaders = Split("""
UserApparatus.h    UserDetector.h     UserEvtHandler.h   UserModule.h       
UserScintillator.h SkeletonModule.h   User_LinkDef.h
""")
baseenv.RootCint(rootuserdict,userheaders)

#######  Start of main SConscript ###########

list = Split("""
UserApparatus.cxx    UserModule.cxx
UserDetector.cxx     UserEvtHandler.cxx   UserScintillator.cxx SkeletonModule.cxx
""")

sotarget = 'User'
srclib = baseenv.SharedLibrary(target = sotarget, source = list+[rootuserdict],\
                               LIBS = [''], LIBPATH = [''])
