###### Hall A Software Podd library SConscript Build File #####
###### Author:  Edward Brash (brash@jlab.org) June 2013
###### Modified for Podd 1.7 directory layout: Ole Hansen (ole@jlab.org) Sep 2018

from podd_util import build_library, write_compiledata
Import('baseenv')

libname = 'Podd'

# Sources and headers
src = """
BankData.cxx            BdataLoc.cxx                 CodaRawDecoder.cxx
DecData.cxx             FileInclude.cxx              FixedArrayVar.cxx
MethodVar.cxx           SeqCollectionMethodVar.cxx   SeqCollectionVar.cxx
SimDecoder.cxx          THaAnalysisObject.cxx        THaAnalyzer.cxx
THaApparatus.cxx        THaArrayString.cxx           THaAvgVertex.cxx
THaBeam.cxx             THaBeamDet.cxx               THaBeamEloss.cxx
THaBeamInfo.cxx         THaBeamModule.cxx            THaBPM.cxx
THaCherenkov.cxx        THaCluster.cxx               THaCodaRun.cxx
THaCoincTime.cxx        THaCut.cxx                   THaCutList.cxx
THaDebugModule.cxx      THaDetectorBase.cxx          THaDetector.cxx
THaDetMap.cxx           THaElectronKine.cxx          THaElossCorrection.cxx
THaEpicsEbeam.cxx       THaEpicsEvtHandler.cxx       THaEvent.cxx
THaEvt125Handler.cxx    THaEvtTypeHandler.cxx        THaExtTarCor.cxx
THaFilter.cxx           THaFormula.cxx               THaGoldenTrack.cxx
THaHelicityDet.cxx      THaIdealBeam.cxx             THaInterface.cxx
THaNamedList.cxx        THaNonTrackingDetector.cxx   THaOutput.cxx
THaParticleInfo.cxx     THaPhotoReaction.cxx         THaPhysicsModule.cxx
THaPidDetector.cxx      THaPIDinfo.cxx               THaPostProcess.cxx
THaPrimaryKine.cxx      THaPrintOption.cxx           THaRaster.cxx
THaRasteredBeam.cxx     THaReacPointFoil.cxx         THaReactionPoint.cxx
THaRTTI.cxx             THaRunBase.cxx               THaRun.cxx
THaRunParameters.cxx    THaSAProtonEP.cxx            THaScalerEvtHandler.cxx
THaScintillator.cxx     THaSecondaryKine.cxx         THaShower.cxx
THaSpectrometer.cxx     THaSpectrometerDetector.cxx  THaString.cxx
THaSubDetector.cxx      THaTextvars.cxx              THaTotalShower.cxx
THaTrack.cxx            THaTrackEloss.cxx            THaTrackID.cxx
THaTrackInfo.cxx        THaTrackingDetector.cxx      THaTrackingModule.cxx
THaTrackOut.cxx         THaTrackProj.cxx             THaTriggerTime.cxx
THaTwoarmVertex.cxx     THaUnRasteredBeam.cxx        THaVar.cxx
THaVarList.cxx          THaVertexModule.cxx          THaVform.cxx
THaVhist.cxx            VariableArrayVar.cxx         Variable.cxx
VectorObjMethodVar.cxx  VectorObjVar.cxx             VectorVar.cxx
"""

# Generate ha_compiledata.h header file
compiledata = 'ha_compiledata.h'
write_compiledata(baseenv,compiledata)

extrahdrs = ['Helper.h','VarDef.h','VarType.h',compiledata]

poddlib = build_library(baseenv, libname, src, extrahdrs,
                        extradicthdrs = ['THaGlobals.h'], useenv = False,
                        versioned = True)
Clean(poddlib, compiledata)
