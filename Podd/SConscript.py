###### Hall A Software Podd library SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013
###### Modified for Podd 1.7 directory layout: Ole Hansen (ole@jlab.org) Sep 2018

from podd_util import build_library, write_compiledata
Import('baseenv')

libname = 'Podd'

# Sources and headers
src = """
BankData.cxx                 BdataLoc.cxx                 CodaRawDecoder.cxx
DecData.cxx                  DetectorData.cxx             FileInclude.cxx
FixedArrayVar.cxx            InterStageModule.cxx         MethodVar.cxx
MultiFileRun.cxx             SeqCollectionMethodVar.cxx   SeqCollectionVar.cxx
SimDecoder.cxx               THaAnalysisObject.cxx        THaAnalyzer.cxx
THaApparatus.cxx             THaArrayString.cxx           THaAvgVertex.cxx
THaBPM.cxx                   THaBeam.cxx                  THaBeamDet.cxx
THaBeamEloss.cxx             THaBeamInfo.cxx              THaBeamModule.cxx
THaCherenkov.cxx             THaCluster.cxx               THaCodaRun.cxx
THaCoincTime.cxx             THaCut.cxx                   THaCutList.cxx
THaDebugModule.cxx           THaDetMap.cxx                THaDetector.cxx
THaDetectorBase.cxx          THaElectronKine.cxx          THaElossCorrection.cxx
THaEpicsEbeam.cxx            THaEpicsEvtHandler.cxx       THaEvent.cxx
THaEvt125Handler.cxx         THaEvtTypeHandler.cxx        THaExtTarCor.cxx
THaFilter.cxx                THaFormula.cxx               THaGoldenTrack.cxx
THaHelicityDet.cxx           THaIdealBeam.cxx             THaInterface.cxx
THaNamedList.cxx             THaNonTrackingDetector.cxx   THaOutput.cxx
THaPIDinfo.cxx               THaParticleInfo.cxx          THaPhotoReaction.cxx
THaPhysicsModule.cxx         THaPidDetector.cxx           THaPostProcess.cxx
THaPrimaryKine.cxx           THaPrintOption.cxx           THaRTTI.cxx
THaRaster.cxx                THaRasteredBeam.cxx          THaReacPointFoil.cxx
THaReactionPoint.cxx         THaRun.cxx                   THaRunBase.cxx
THaRunParameters.cxx         THaSAProtonEP.cxx            THaScalerEvtHandler.cxx
THaScintillator.cxx          THaSecondaryKine.cxx         THaShower.cxx
THaSpectrometer.cxx          THaSpectrometerDetector.cxx  THaString.cxx
THaSubDetector.cxx           THaTotalShower.cxx           THaTrack.cxx
THaTrackEloss.cxx            THaTrackID.cxx               THaTrackInfo.cxx
THaTrackOut.cxx              THaTrackProj.cxx             THaTrackingDetector.cxx
THaTrackingModule.cxx        THaTriggerTime.cxx           THaTwoarmVertex.cxx
THaUnRasteredBeam.cxx        THaVar.cxx                   THaVarList.cxx
THaVertexModule.cxx          THaVform.cxx                 THaVhist.cxx
TimeCorrectionModule.cxx     Variable.cxx                 VariableArrayVar.cxx
VectorObjMethodVar.cxx       VectorObjVar.cxx             VectorVar.cxx
"""

# Generate ha_compiledata.h header file
compiledata = 'ha_compiledata.h'
write_compiledata(baseenv,compiledata)

extrahdrs = ['DataType.h','OptionalType.h','optional.hpp',compiledata]

poddlib = build_library(baseenv, libname, src, extrahdrs,
                        extradicthdrs = ['THaGlobals.h'], useenv = False,
                        versioned = True)
Clean(poddlib, compiledata)
