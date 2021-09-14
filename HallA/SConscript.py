###### Hall A Software HallA library SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013
###### Modified for Podd 1.7 directory layout: Ole Hansen (ole@jlab.org) Sep 2018

from podd_util import build_library

Import('baseenv')

libname = 'HallA'

# Sources and headers
src = """
FADCData.cxx               FadcBPM.cxx                 FadcCherenkov.cxx
FadcRaster.cxx             FadcRasteredBeam.cxx        FadcUnRasteredBeam.cxx
FadcScintillator.cxx       FadcShower.cxx
THaADCHelicity.cxx         THaDecData.cxx              THaG0Helicity.cxx
THaG0HelicityReader.cxx    THaHelicity.cxx             THaHRS.cxx
THaQWEAKHelicity.cxx       THaQWEAKHelicityReader.cxx  THaS2CoincTime.cxx
THaVDCAnalyticTTDConv.cxx  THaVDCChamber.cxx           THaVDCCluster.cxx
THaVDC.cxx                 THaVDCHit.cxx               THaVDCPlane.cxx
THaVDCPoint.cxx            THaVDCPointPair.cxx         THaVDCTimeToDistConv.cxx
THaVDCTrackID.cxx          THaVDCWire.cxx              TrigBitLoc.cxx
VDCeff.cxx                 TwoarmVDCTimeCorrection.cxx
"""

build_library(baseenv, libname, src, useenv = False, versioned = True)
