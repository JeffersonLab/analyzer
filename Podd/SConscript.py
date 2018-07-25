###### Hall A Software src SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013

import os
import re
import SCons.Util
Import('baseenv')

list = Split("""
THaADCHelicity.cxx          THaCoincTime.cxx            THaG0Helicity.cxx
THaPhotoReaction.cxx        THaRunParameters.cxx        THaTrackID.cxx
THaVDCTrackID.cxx           THaAnalysisObject.cxx       THaCut.cxx
THaG0HelicityReader.cxx     THaPhysicsModule.cxx        THaS2CoincTime.cxx
THaTrackInfo.cxx            THaAnalyzer.cxx             THaCutList.cxx
THaGoldenTrack.cxx          THaPidDetector.cxx          THaSAProtonEP.cxx
THaTrackOut.cxx             THaVDCPointPair.cxx         THaApparatus.cxx
THaDebugModule.cxx          THaHRS.cxx                  THaVDCPoint.cxx
THaPostProcess.cxx          THaTrackProj.cxx            THaEpicsEvtHandler.cxx
THaVDCChamber.cxx           THaArrayString.cxx          THaDecData.cxx
BdataLoc.cxx                THaHelicity.cxx             THaPrimaryKine.cxx
THaScintillator.cxx         THaTrackingDetector.cxx     THaVDCWire.cxx
THaAvgVertex.cxx            THaDetMap.cxx               THaHelicityDet.cxx
THaPrintOption.cxx          THaSecondaryKine.cxx        THaTrackingModule.cxx
THaVar.cxx                  THaBPM.cxx                  THaDetector.cxx
THaIdealBeam.cxx            THaQWEAKHelicity.cxx        THaShower.cxx
THaTriggerTime.cxx          THaVarList.cxx              THaBeam.cxx
THaDetectorBase.cxx         THaInterface.cxx
THaQWEAKHelicityReader.cxx
THaSpectrometer.cxx         THaTwoarmVertex.cxx         THaVertexModule.cxx
THaBeamDet.cxx              THaElectronKine.cxx         THaNamedList.cxx
THaRTTI.cxx                 THaSpectrometerDetector.cxx THaUnRasteredBeam.cxx
THaVform.cxx                THaBeamEloss.cxx            THaElossCorrection.cxx
THaNonTrackingDetector.cxx  THaRaster.cxx               THaString.cxx
THaVDC.cxx                  THaVhist.cxx                THaBeamInfo.cxx
THaEpicsEbeam.cxx           THaRasteredBeam.cxx         THaSubDetector.cxx
THaVDCAnalyticTTDConv.cxx   VDCeff.cxx                  THaBeamModule.cxx
THaEvent.cxx                FileInclude.cxx             THaReacPointFoil.cxx
THaTextvars.cxx             THaVDCCluster.cxx           THaCherenkov.cxx
THaExtTarCor.cxx            THaOutput.cxx               THaReactionPoint.cxx
THaTotalShower.cxx          THaVDCHit.cxx               THaCluster.cxx
THaFilter.cxx               THaPIDinfo.cxx              THaRun.cxx
THaTrack.cxx                THaVDCPlane.cxx             THaCodaRun.cxx
THaFormula.cxx              THaParticleInfo.cxx         THaRunBase.cxx
THaTrackEloss.cxx           THaVDCTimeToDistConv.cxx    THaEvtTypeHandler.cxx
THaScalerEvtHandler.cxx     THaEvt125Handler.cxx        Variable.cxx
VariableArrayVar.cxx        FixedArrayVar.cxx           VectorVar.cxx
MethodVar.cxx               SeqCollectionVar.cxx 
SeqCollectionMethodVar.cxx  VectorObjVar.cxx            VectorObjMethodVar.cxx
BankData.cxx
""")

baseenv.Object('main.cxx')

sotarget = 'HallA'

srclib = baseenv.SharedLibrary(target = sotarget,\
             source = list+[baseenv.subst('$MAIN_DIR')+'/haDict.cxx'],\
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
