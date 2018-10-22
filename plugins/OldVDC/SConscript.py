###### Hall A Old VDC library SConscript File #####

from podd_util import build_library
Import ('baseenv')

libname = 'OldVDC'

src = """
OldVDC.cxx OldVDCPlane.cxx OldVDCUVPlane.cxx OldVDCUVTrack.cxx
OldVDCHit.cxx OldVDCCluster.cxx  OldVDCWire.cxx OldVDCTrackID.cxx
OldVDCTrackPair.cxx OldVDCTimeToDistConv.cxx OldVDCAnalyticTTDConv.cxx
"""

build_library(baseenv, libname, src)
