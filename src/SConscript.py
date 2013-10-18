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
THaPhysicsModule.C        THaS2CoincTime.C          THaTrackInfo.C            THaVDCTrackPair.C
THaAnalyzer.C             THaCutList.C              THaGoldenTrack.C          
THaPidDetector.C          THaSAProtonEP.C           THaTrackOut.C             THaVDCUVPlane.C
THaApparatus.C            THaDebugModule.C          THaHRS.C                  
THaPostProcess.C          THaScalerGroup.C          THaTrackProj.C            THaVDCUVTrack.C
THaArrayString.C          THaDecData.C              THaHelicity.C             
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
THaRasteredBeam.C         THaSubDetector.C          THaVDCAnalyticTTDConv.C
THaBeamModule.C           THaEvent.C               
THaReacPointFoil.C        THaTextvars.C             THaVDCCluster.C
THaCherenkov.C            THaExtTarCor.C            THaOutput.C               
THaReactionPoint.C        THaTotalShower.C          THaVDCHit.C
THaCluster.C              THaFilter.C               THaPIDinfo.C              
THaRun.C                  THaTrack.C                THaVDCPlane.C
THaCodaRun.C              THaFormula.C              THaParticleInfo.C         
THaRunBase.C              THaTrackEloss.C           THaVDCTimeToDistConv.C
""")

normanalist = ['THaNormAna.C']

baseenv.Object('main.C')

sotarget = 'HallA'
normanatarget = 'NormAna'

srclib = baseenv.SharedLibrary(target = sotarget, source = list+['haDict.so'],SHLIBVERSION=['$VERSION'],SHLIBPREFIX='../lib',LIBS=[''])
print ('Source shared library = %s\n' % srclib)
#
if baseenv['PLATFORM'] == 'darwin':
	cleantarget = '../'+baseenv.subst('$SHLIBPREFIX')+sotarget+'.'+baseenv.subst('$VERSION')+baseenv.subst('$SHLIBSUFFIX')
else:
	cleantarget = '../'+baseenv.subst('$SHLIBPREFIX')+sotarget+baseenv.subst('$SHLIBSUFFIX')+'.'+baseenv.subst('$VERSION')
Clean(srclib,cleantarget)
shortcleantarget = '../'+baseenv.subst('$SHLIBPREFIX')+sotarget+baseenv.subst('$SHLIBSUFFIX')+'.'+baseenv.subst('$SOVERSION')
Clean(srclib,shortcleantarget)
#
#baseenv.Install('../lib',srclib)
#baseenv.Alias('install',['../lib'])

normanalib = baseenv.SharedLibrary(target = normanatarget,source = normanalist+['NormAnaDict.so'],SHLIBVERSION=['$VERSION'],SHLIBPREFIX='../lib',LIBS=[''])
print ('NormAna shared library = %s\n' % normanalib)
#
if baseenv['PLATFORM'] == 'darwin':
	normanacleantarget = '../'+baseenv.subst('$SHLIBPREFIX')+normanatarget+'.'+baseenv.subst('$VERSION')+baseenv.subst('$SHLIBSUFFIX')
else:
	normanacleantarget = '../'+baseenv.subst('$SHLIBPREFIX')+normanatarget+baseenv.subst('$SHLIBSUFFIX')+'.'+baseenv.subst('$VERSION')
Clean(srclib,normanacleantarget)
shortnormanacleantarget = '../'+baseenv.subst('$SHLIBPREFIX')+normanatarget+baseenv.subst('$SHLIBSUFFIX')+'.'+baseenv.subst('$SOVERSION')
Clean(srclib,shortnormanacleantarget)
#baseenv.Install('../lib',normanalib)
#baseenv.Alias('install',['../lib'])

