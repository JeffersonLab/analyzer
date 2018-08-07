###### Hall A Software Main SConscript File #####
###### Author:	Edward Brash (brash@jlab.org) June 2013

import os
import re
import sys
import subprocess
import platform
import time
import SCons.Util
Import ('baseenv')

######## ROOT Dictionaries #########

rootdecdict = baseenv.subst('$MAIN_DIR')+'/THaDecDict.cxx'
decheaders = Split("""
hana_decode/THaUsrstrutils.h hana_decode/THaCrateMap.h
hana_decode/THaCodaData.h hana_decode/THaEpics.h
hana_decode/THaCodaFile.h hana_decode/SimDecoder.h
hana_decode/THaSlotData.h hana_decode/THaEvData.h
hana_decode/CodaDecoder.h hana_decode/Module.h hana_decode/VmeModule.h
hana_decode/FastbusModule.h hana_decode/Lecroy1877Module.h
hana_decode/Lecroy1881Module.h hana_decode/Lecroy1875Module.h
hana_decode/Fadc250Module.h hana_decode/GenScaler.h
hana_decode/Scaler560.h hana_decode/Scaler1151.h
hana_decode/Scaler3800.h hana_decode/Scaler3801.h
hana_decode/F1TDCModule.h hana_decode/Caen1190Module.h
hana_decode/Caen775Module.h hana_decode/Caen792Module.h
hana_decode/THaBenchmark.h hana_decode/haDecode_LinkDef.h
""")
baseenv.RootCint(rootdecdict,decheaders)
baseenv.Clean(rootdecdict,re.sub(r'\.cxx\Z','_rdict.pcm',rootdecdict))

roothadict = baseenv.subst('$MAIN_DIR')+'/haDict.cxx'
haheaders = Split("""
src/THaFormula.h src/THaVform.h src/THaVhist.h src/THaVar.h
src/THaVarList.h src/THaCut.h src/THaNamedList.h src/THaCutList.h
src/THaInterface.h src/THaRunBase.h src/THaCodaRun.h src/THaRun.h
src/THaRunParameters.h src/THaDetMap.h src/THaApparatus.h
src/THaDetector.h src/THaSpectrometer.h src/THaSpectrometerDetector.h
src/THaHRS.h src/THaDecData.h src/BdataLoc.h src/THaOutput.h src/BankData.h
src/THaString.h src/THaTrackingDetector.h src/THaNonTrackingDetector.h
src/THaPidDetector.h src/THaSubDetector.h src/THaAnalysisObject.h
src/THaDetectorBase.h src/THaRTTI.h src/THaPhysicsModule.h
src/THaVertexModule.h src/THaTrackingModule.h src/THaAnalyzer.h
src/THaPrintOption.h src/THaBeam.h src/THaIdealBeam.h
src/THaRasteredBeam.h src/THaRaster.h src/THaBeamDet.h src/THaBPM.h
src/THaUnRasteredBeam.h src/THaTrack.h src/THaPIDinfo.h
src/THaParticleInfo.h src/THaCluster.h src/THaArrayString.h
src/THaScintillator.h src/THaShower.h src/THaTotalShower.h
src/THaCherenkov.h src/THaEvent.h src/THaTrackID.h src/THaVDC.h
src/THaVDCPlane.h src/THaVDCWire.h src/THaVDCHit.h src/THaVDCCluster.h
src/THaVDCTimeToDistConv.h src/THaVDCTrackID.h
src/THaVDCAnalyticTTDConv.h src/VDCeff.h src/THaElectronKine.h
src/THaReactionPoint.h src/THaReacPointFoil.h src/THaTwoarmVertex.h
src/THaAvgVertex.h src/THaExtTarCor.h src/THaDebugModule.h
src/THaTrackInfo.h src/THaGoldenTrack.h src/THaPrimaryKine.h
src/THaSecondaryKine.h src/THaCoincTime.h src/THaS2CoincTime.h
src/THaTrackProj.h src/THaPostProcess.h src/THaFilter.h
src/THaElossCorrection.h src/THaTrackEloss.h src/THaBeamModule.h
src/THaBeamInfo.h src/THaEpicsEbeam.h src/THaBeamEloss.h
src/THaTrackOut.h src/THaTriggerTime.h src/THaHelicityDet.h
src/THaG0HelicityReader.h src/THaG0Helicity.h src/THaADCHelicity.h
src/THaHelicity.h src/THaPhotoReaction.h src/THaSAProtonEP.h
src/THaTextvars.h src/THaQWEAKHelicity.h src/THaQWEAKHelicityReader.h
src/THaEvtTypeHandler.h src/THaScalerEvtHandler.h
src/THaEpicsEvtHandler.h src/THaEvt125Handler.h src/Variable.h
src/VariableArrayVar.h src/FixedArrayVar.h src/VectorVar.h
src/MethodVar.h src/SeqCollectionVar.h src/SeqCollectionMethodVar.h
src/VectorObjVar.h src/VectorObjMethodVar.h src/THaVDCChamber.h
src/THaVDCPoint.h src/THaVDCPointPair.h src/THaGlobals.h
src/FileInclude.h src/HallA_LinkDef.h
""")
baseenv.RootCint(roothadict,haheaders)
baseenv.Clean(roothadict,re.sub(r'\.cxx\Z','_rdict.pcm',roothadict))

#######  write src/ha_compiledata.h header file ######

if sys.version_info >= (2, 7):
    try:
        cmd = "git rev-parse HEAD 2>/dev/null"
        gitrev = subprocess.check_output(cmd, shell=True).rstrip()
    except:
        gitrev = ''
    try:
        cmd = baseenv.subst('$CXX') + " --version 2>/dev/null | head -1"
        cxxver = subprocess.check_output(cmd, shell=True).rstrip()
    except:
        cxxver = ''
    # subprocess gives us byte string literals in Python 3, but we'd like
    # Unicode strings
    if sys.version_info >= (3, 0):
        gitrev = gitrev.decode()
        cxxver = cxxver.decode()
else:
    FNULL = open(os.devnull, 'w')
    try:
        gitrev = subprocess.Popen(['git', 'rev-parse', 'HEAD', '2>dev/null'],\
                    stdout=subprocess.PIPE, stderr=FNULL).communicate()[0].rstrip()
    except:
        gitrev =''
    try:
        outp = subprocess.Popen([baseenv.subst('$CXX'), '--version'],\
                                stdout=subprocess.PIPE, stderr=FNULL).communicate()[0]
        lines = outp.splitlines()
        cxxver = lines[0]
    except:
        cxxver = ''

compiledata = 'src/ha_compiledata.h'
f=open(compiledata,'w')
f.write('#ifndef ANALYZER_COMPILEDATA_H\n')
f.write('#define ANALYZER_COMPILEDATA_H\n')
f.write('\n')
f.write('#define HA_INCLUDEPATH "%s %s"\n' % (baseenv.subst('$HA_SRC'),baseenv.subst('$HA_DC')))
f.write('#define HA_VERSION "%s"\n' % baseenv.subst('$HA_VERSION'))
f.write('#define HA_DATE "%s"\n' % time.strftime("%b %d %Y"))
f.write('#define HA_DATETIME "%s"\n' % time.strftime("%a %b %d %Y"))
#f.write('#define HA_DATETIME "%s"\n' % time.strftime("%a %b %d %H:%M:%S %Z %Y"))
f.write('#define HA_PLATFORM "%s"\n' % platform.platform())
f.write('#define HA_BUILDNODE "%s"\n' % platform.node())
f.write('#define HA_BUILDDIR "%s"\n' % os.getcwd())
try:
    builduser = baseenv['ENV']['LOGNAME']
except:
    builduser = ''
f.write('#define HA_BUILDUSER "%s"\n' % builduser)
f.write('#define HA_GITREV "%s"\n' % gitrev[:7])
f.write('#define HA_CXXVERS "%s"\n' % cxxver)
f.write('#define HA_ROOTVERS "%s"\n' % baseenv.subst('$ROOTVERS'))
f.write('#define ANALYZER_VERSION_CODE %s\n' % baseenv.subst('$VERCODE'))
f.write('#define ANALYZER_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))\n')
f.write('\n')
f.write('#endif\n')
f.close()

#######  Start of main SConscript ###########

analyzer = baseenv.Program(target = 'analyzer', source = 'src/main.cxx')
baseenv.Install('./bin',analyzer)
baseenv.Alias('install',['./bin'])
baseenv.Clean(analyzer,compiledata)
