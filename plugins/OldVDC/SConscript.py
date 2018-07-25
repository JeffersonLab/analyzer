###### Hall A Old VDC library SConscript File #####

import re

Import ('baseenv')

######## ROOT Dictionaries #########

rootuserdict = baseenv.subst('$MAIN_DIR')+'/OldVDCDict.cxx'
userheaders = Split("""
OldVDC.h OldVDCPlane.h OldVDCUVPlane.h OldVDCUVTrack.h
OldVDCHit.h OldVDCCluster.h  OldVDCWire.h OldVDCTrackID.h
OldVDCTrackPair.h OldVDCTimeToDistConv.h OldVDCAnalyticTTDConv.h
OldVDC_LinkDef.h
""")
baseenv.RootCint(rootuserdict,userheaders)
baseenv.Clean(rootuserdict,re.sub(r'\.cxx\Z','_rdict.pcm',rootuserdict))

#######  Start of main SConscript ###########

list = Split("""
OldVDC.cxx OldVDCPlane.cxx OldVDCUVPlane.cxx OldVDCUVTrack.cxx
OldVDCHit.cxx OldVDCCluster.cxx  OldVDCWire.cxx OldVDCTrackID.cxx
OldVDCTrackPair.cxx OldVDCTimeToDistConv.cxx OldVDCAnalyticTTDConv.cxx
""")

sotarget = 'OldVDC'
srclib = baseenv.SharedLibrary(target = sotarget, source = list+[rootuserdict],\
                               LIBS = [''], LIBPATH = [''])
