###### Hall A Software Main SConscript File #####
###### Author:	Edward Brash (brash@jlab.org) June 2013

import os
import re
import SCons.Util
Import ('baseenv')

######## ROOT Dictionaries #########

rootnormanadict = baseenv.subst('$MAIN_DIR')+'/NormAnaDict.C'
rootnormanaobj = baseenv.subst('$HA_SRC')+'/NormAnaDict.so'
normanaheaders = Split("""
src/THaNormAna.h src/NormAna_LinkDef.h
""")
baseenv.RootCint(rootnormanadict,normanaheaders)
baseenv.SharedObject(target = rootnormanaobj, source = rootnormanadict)

rootscaldict = baseenv.subst('$MAIN_DIR')+'/THaScalDict.C'
rootscalobj = baseenv.subst('$HA_SCALER')+'/THaScalDict.so'
scalheaders = Split("""
hana_scaler/THaScaler.h hana_scaler/THaScalerDB.h hana_scaler/haScal_LinkDef.h
""")
baseenv.RootCint(rootscaldict,scalheaders)
baseenv.SharedObject(target = rootscalobj, source = rootscaldict)

rootdecdict = baseenv.subst('$MAIN_DIR')+'/THaDecDict.C'
rootdecobj = baseenv.subst('$HA_DC')+'/THaDecDict.so'
decheaders = Split("""
hana_decode/THaUsrstrutils.h hana_decode/THaCrateMap.h hana_decode/THaCodaData.h hana_decode/THaEpics.h
hana_decode/THaFastBusWord.h hana_decode/THaCodaFile.h hana_decode/THaSlotData.h hana_decode/THaEvData.h
hana_decode/THaCodaDecoder.h hana_decode/SimDecoder.h
hana_decode/CodaDecoder.h hana_decode/Module.h hana_decode/VmeModule.h
hana_decode/FastbusModule.h hana_decode/Lecroy1877Module.h
hana_decode/Lecroy1881Module.h hana_decode/Lecroy1875Module.h
hana_decode/Fadc250Module.h hana_decode/GenScaler.h hana_decode/Scaler560.h
hana_decode/Scaler1151.h hana_decode/Scaler3800.h hana_decode/Scaler3801.h
hana_decode/F1TDCModule.h hana_decode/SkeletonModule.h
hana_decode/THaBenchmark.h hana_decode/haDecode_LinkDef.h
""")
#hana_decode/evio.h
baseenv.RootCint(rootdecdict,decheaders)
baseenv.SharedObject(target = rootdecobj, source = rootdecdict)

roothadict = baseenv.subst('$MAIN_DIR')+'/haDict.C'
roothaobj = baseenv.subst('$HA_SRC')+'/haDict.so'
haheaders = Split("""
src/THaFormula.h src/THaVform.h src/THaVhist.h src/THaVar.h src/THaVarList.h 
src/THaCut.h src/THaNamedList.h src/THaCutList.h src/THaInterface.h src/THaRunBase.h 
src/THaCodaRun.h src/THaRun.h src/THaRunParameters.h src/THaDetMap.h src/THaApparatus.h 
src/THaDetector.h src/THaSpectrometer.h src/THaSpectrometerDetector.h src/THaHRS.h 
src/THaDecData.h src/BdataLoc.h src/THaOutput.h src/THaString.h src/THaTrackingDetector.h 
src/THaNonTrackingDetector.h src/THaPidDetector.h src/THaSubDetector.h 
src/THaAnalysisObject.h src/THaDetectorBase.h src/THaRTTI.h src/THaPhysicsModule.h 
src/THaVertexModule.h src/THaTrackingModule.h src/THaAnalyzer.h src/THaPrintOption.h 
src/THaBeam.h src/THaIdealBeam.h src/THaRasteredBeam.h src/THaRaster.h src/THaBeamDet.h 
src/THaBPM.h src/THaUnRasteredBeam.h src/THaTrack.h src/THaPIDinfo.h 
src/THaParticleInfo.h src/THaCluster.h src/THaArrayString.h src/THaScintillator.h 
src/THaShower.h src/THaTotalShower.h src/THaCherenkov.h src/THaEvent.h 
src/THaTrackID.h src/THaVDC.h src/THaVDCPlane.h src/THaVDCUVPlane.h src/THaVDCUVTrack.h 
src/THaVDCWire.h src/THaVDCHit.h src/THaVDCCluster.h src/THaVDCTimeToDistConv.h 
src/THaVDCTrackID.h src/THaVDCAnalyticTTDConv.h src/THaVDCTrackPair.h src/VDCeff.h
src/THaScalerGroup.h src/THaElectronKine.h src/THaReactionPoint.h 
src/THaReacPointFoil.h src/THaTwoarmVertex.h src/THaAvgVertex.h src/THaExtTarCor.h 
src/THaDebugModule.h src/THaTrackInfo.h src/THaGoldenTrack.h src/THaPrimaryKine.h 
src/THaSecondaryKine.h src/THaCoincTime.h src/THaS2CoincTime.h src/THaTrackProj.h 
src/THaPostProcess.h src/THaFilter.h src/THaElossCorrection.h src/THaTrackEloss.h 
src/THaBeamModule.h src/THaBeamInfo.h src/THaEpicsEbeam.h src/THaBeamEloss.h 
src/THaTrackOut.h src/THaTriggerTime.h src/THaHelicityDet.h src/THaG0HelicityReader.h 
src/THaG0Helicity.h src/THaADCHelicity.h src/THaHelicity.h src/THaPhotoReaction.h 
src/THaSAProtonEP.h src/THaTextvars.h src/THaQWEAKHelicity.h src/THaQWEAKHelicityReader.h
src/THaEvtTypeHandler.h src/THaScalerEvtHandler.h
src/THaGlobals.h src/HallA_LinkDef.h
""")
baseenv.RootCint(roothadict,haheaders)
baseenv.SharedObject(target = roothaobj, source = roothadict)

#######  write src/ha_compiledata.h header file ######

f=open('src/ha_compiledata.h','w')
f.write('#ifndef ANALYZER_COMPILEDATA_H\n')
f.write('#define ANALYZER_COMPILEDATA_H\n')
f.write('\n')
f.write('#define HA_INCLUDEPATH "%s %s %s"\n' % (baseenv.subst('$HA_SRC'),baseenv.subst('$HA_DC'),baseenv.subst('$HA_SCALER'))) 
f.write('#define HA_VERSION "%s"\n' % baseenv.subst('$HA_VERSION'))
f.write('#define ANALYZER_VERSION_CODE %s\n' % baseenv.subst('$VERCODE'))
f.write('#define ANALYZER_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))\n')
f.write('\n')
f.write('#endif\n')
f.close() 

#######  Start of main SConscript ###########

analyzer = baseenv.Program(target = 'analyzer', source = 'src/main.o')
baseenv.Install('./bin',analyzer)
baseenv.Alias('install',['./bin'])
