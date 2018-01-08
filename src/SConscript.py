###### Hall A Software src SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013

import os
import re
import SCons.Util
Import('baseenv')

list = Split("""
THaADCHelicity.C          THaCoincTime.C            THaG0Helicity.C
THaPhotoReaction.C        THaRunParameters.C        THaTrackID.C              THaVDCTrackID.C
THaAnalysisObject.C       THaCut.C                  THaG0HelicityReader.C
THaPhysicsModule.C        THaS2CoincTime.C          THaTrackInfo.C
THaAnalyzer.C             THaCutList.C              THaGoldenTrack.C
THaPidDetector.C          THaSAProtonEP.C           THaTrackOut.C             THaVDCPointPair.C
THaApparatus.C            THaDebugModule.C          THaHRS.C                  THaVDCPoint.C
THaPostProcess.C          THaTrackProj.C            THaEpicsEvtHandler.C      THaVDCChamber.C
THaArrayString.C          THaDecData.C              BdataLoc.C                THaHelicity.C
THaPrimaryKine.C          THaScintillator.C         THaTrackingDetector.C     THaVDCWire.C
THaAvgVertex.C            THaDetMap.C               THaHelicityDet.C
THaPrintOption.C          THaSecondaryKine.C        THaTrackingModule.C       THaVar.C
THaBPM.C                  THaDetector.C             THaIdealBeam.C
THaQWEAKHelicity.C        THaShower.C               THaTriggerTime.C          THaVarList.C
THaBeam.C                 THaDetectorBase.C         THaInterface.C
THaQWEAKHelicityReader.C  THaSpectrometer.C         THaTwoarmVertex.C         THaVertexModule.C
THaBeamDet.C              THaElectronKine.C         THaNamedList.C
THaRTTI.C                 THaSpectrometerDetector.C THaUnRasteredBeam.C       THaVform.C
THaBeamEloss.C            THaElossCorrection.C      THaNonTrackingDetector.C
THaRaster.C               THaString.C               THaVDC.C                  THaVhist.C
THaBeamInfo.C             THaEpicsEbeam.C
THaRasteredBeam.C         THaSubDetector.C          THaVDCAnalyticTTDConv.C   VDCeff.C
THaBeamModule.C           THaEvent.C                FileInclude.C
THaReacPointFoil.C        THaTextvars.C             THaVDCCluster.C
THaCherenkov.C            THaExtTarCor.C            THaOutput.C
THaReactionPoint.C        THaTotalShower.C          THaVDCHit.C
THaCluster.C              THaFilter.C               THaPIDinfo.C
THaRun.C                  THaTrack.C                THaVDCPlane.C
THaCodaRun.C              THaFormula.C              THaParticleInfo.C
THaRunBase.C              THaTrackEloss.C           THaVDCTimeToDistConv.C
THaEvtTypeHandler.C       THaScalerEvtHandler.C     THaEvt125Handler.C
""")

baseenv.Object('main.C')

sotarget = 'HallA'

srclib = baseenv.SharedLibrary(target = sotarget,\
             source = list+[baseenv.subst('$MAIN_DIR')+'/haDict.C'],\
             SHLIBPREFIX = baseenv.subst('$MAIN_DIR')+'/lib',\
             LIBS = [''], LIBPATH = [''])
#print ('Source shared library = %s\n' % srclib)

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

Clean(srclib,cleantarget)
Clean(srclib,localmajorcleantarget)
Clean(srclib,localshortcleantarget)
