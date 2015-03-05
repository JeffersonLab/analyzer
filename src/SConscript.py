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
THaBeamModule.C           THaEvent.C               
THaReacPointFoil.C        THaTextvars.C             THaVDCCluster.C
THaCherenkov.C            THaExtTarCor.C            THaOutput.C               
THaReactionPoint.C        THaTotalShower.C          THaVDCHit.C
THaCluster.C              THaFilter.C               THaPIDinfo.C              
THaRun.C                  THaTrack.C                THaVDCPlane.C
THaCodaRun.C              THaFormula.C              THaParticleInfo.C         
THaRunBase.C              THaTrackEloss.C           THaVDCTimeToDistConv.C
THaEvtTypeHandler.C       THaScalerEvtHandler.C
""")

normanalist = ['THaNormAna.C']

baseenv.Object('main.C')

sotarget = 'HallA'
normanatarget = 'NormAna'

#srclib = baseenv.SharedLibrary(target = sotarget, source = list+['haDict.so'],SHLIBVERSION=['$VERSION'],SHLIBPREFIX='../lib',LIBS=[''])
srclib = baseenv.SharedLibrary(target = sotarget, source = list+['haDict.so'],SHLIBPREFIX='../lib',LIBS=[''])
print ('Source shared library = %s\n' % srclib)

linkbase = baseenv.subst('$SHLIBPREFIX')+sotarget

cleantarget = linkbase+'.so.'+baseenv.subst('$VERSION')
majorcleantarget = linkbase+'.so'
localmajorcleantarget = '../'+linkbase+'.so'
shortcleantarget = linkbase+'.so.'+baseenv.subst('$SOVERSION')
localshortcleantarget = '../'+linkbase+'.so.'+baseenv.subst('$SOVERSION')

print ('cleantarget = %s\n' % cleantarget)
print ('majorcleantarget = %s\n' % majorcleantarget)
print ('shortcleantarget = %s\n' % shortcleantarget)
try:
	os.symlink(cleantarget,localshortcleantarget)
	os.symlink(shortcleantarget,localmajorcleantarget)
except:	
	print " Continuing ... "

Clean(srclib,cleantarget)
Clean(srclib,localmajorcleantarget)
Clean(srclib,localshortcleantarget)

#baseenv.Install('../lib',srclib)
#baseenv.Alias('install',['../lib'])

#normanalib = baseenv.SharedLibrary(target = normanatarget,source = normanalist+['NormAnaDict.so'],SHLIBVERSION=['$VERSION'],SHLIBPREFIX='../lib',LIBS=[''])
normanalib = baseenv.SharedLibrary(target = normanatarget,source = normanalist+['NormAnaDict.so'],SHLIBPREFIX='../lib',LIBS=[''])
print ('NormAna shared library = %s\n' % normanalib)

nlinkbase = baseenv.subst('$SHLIBPREFIX')+normanatarget

ncleantarget = nlinkbase+'.so.'+baseenv.subst('$VERSION')
nmajorcleantarget = nlinkbase+'.so'
nlocalmajorcleantarget = '../'+nlinkbase+'.so'
nshortcleantarget = nlinkbase+'.so.'+baseenv.subst('$SOVERSION')
nlocalshortcleantarget = '../'+nlinkbase+'.so.'+baseenv.subst('$SOVERSION')

print ('ncleantarget = %s\n' % ncleantarget)
print ('nmajorcleantarget = %s\n' % nmajorcleantarget)
print ('nshortcleantarget = %s\n' % nshortcleantarget)
try:
	os.symlink(ncleantarget,nlocalshortcleantarget)
	os.symlink(nshortcleantarget,nlocalmajorcleantarget)
except:	
	print " Continuing ... "

Clean(normanalib,ncleantarget)
Clean(normanalib,nlocalmajorcleantarget)
Clean(normanalib,nlocalshortcleantarget)

#baseenv.Install('../lib',normanalib)
#baseenv.Alias('install',['../lib'])

