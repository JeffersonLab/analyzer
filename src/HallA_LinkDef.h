#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ global gHaVars;
#pragma link C++ global gHaCuts;
#pragma link C++ global gHaApps;
#pragma link C++ global gHaScalers;
#pragma link C++ global gHaPhysics;
#pragma link C++ global gHaRun;

#pragma link C++ class THaVar+;
#pragma link C++ class THaVarList+;
#pragma link C++ class THaNamedList+;
#pragma link C++ class THaFormula+;
#pragma link C++ class THaVform+;
#pragma link C++ class THaVhist+;
#pragma link C++ class THaCut+;
#pragma link C++ class THaCutList+;
#pragma link C++ class THaHashList+;
#pragma link C++ class THaInterface+;
#pragma link C++ class THaRun+;
#pragma link C++ class THaRunBase+;
#pragma link C++ class THaCodaRun+;
#pragma link C++ class THaRunParameters+;
#pragma link C++ class THaApparatus+;
#pragma link C++ class THaSpectrometer+;
#pragma link C++ class THaHRS+;
#pragma link C++ class THaDecData+;
#pragma link C++ class THaAnalysisObject+;
#pragma link C++ class THaDetectorBase+;
#pragma link C++ class THaPhysicsModule+;
#pragma link C++ class THaVertexModule+;
#pragma link C++ class THaTrackingModule+;
#pragma link C++ class THaDetector+;
#pragma link C++ class THaSubDetector+;
#pragma link C++ class THaSpectrometerDetector+;
#pragma link C++ class THaTrackingDetector+;
#pragma link C++ class THaNonTrackingDetector+;
#pragma link C++ class THaPidDetector+;
#pragma link C++ class THaDetMap+;
#pragma link C++ class THaScintillator+;
#pragma link C++ class THaEvent+;
#pragma link C++ class THaEventHeader+;
#pragma link C++ class THaOutput+;
#pragma link C++ class THaOdata+;
#pragma link C++ class THaScalerKey+;
#pragma link C++ class THaString+;
#pragma link C++ class THaAnalyzer+;
#pragma link C++ class THaPrintOption+;
#pragma link C++ class THaBeam+;
#pragma link C++ class THaBeamDet+;
#pragma link C++ class THaIdealBeam+;
#pragma link C++ class THaRasteredBeam+;
#pragma link C++ class THaUnRasteredBeam+;
#pragma link C++ class THaRaster+;
#pragma link C++ class THaBPM+;
#pragma link C++ class THaShower+;
#pragma link C++ class THaTrack+;
#pragma link C++ class THaTrackID+;
#pragma link C++ class THaPIDinfo+;
#pragma link C++ class THaParticleInfo+;
#pragma link C++ class THaCluster+;
#pragma link C++ class THaMatrix+;
#pragma link C++ class THaArrayString+;
#pragma link C++ class THaCherenkov+;
#pragma link C++ class THaTotalShower+;
#pragma link C++ class THaVDC+;
#pragma link C++ class THaVDCUVPlane+;
#pragma link C++ class THaVDCPlane+;
#pragma link C++ class THaVDCCluster+;
#pragma link C++ class THaVDCHit+;
#pragma link C++ class THaVDCWire+;
#pragma link C++ class THaVDCUVTrack+;
#pragma link C++ class THaVDCTimeToDistConv+;
#pragma link C++ class THaVDCAnalyticTTDConv+;
#pragma link C++ class THaVDCTrackID+;
#pragma link C++ class THaVDCTrackPair+;
#pragma link C++ class THaRTTI+;
#pragma link C++ class THaRawEvent+;
#pragma link C++ class THaVDCEvent+;
#pragma link C++ class THaScalerGroup+;
#pragma link C++ class THaElectronKine+;
#pragma link C++ class THaReactionPoint+;
#pragma link C++ class THaTwoarmVertex+;
#pragma link C++ class THaAvgVertex+;
#pragma link C++ class THaExtTarCor+;
#pragma link C++ class THaTrackInfo+;
#pragma link C++ class THaDebugModule+;
#pragma link C++ class THaGoldenTrack+;
#pragma link C++ class THaPrimaryKine+;
#pragma link C++ class THaSecondaryKine+;
#pragma link C++ class THaDB+;
#pragma link C++ class THaDetConfig+;
#pragma link C++ class THaDBFile+;
#pragma link C++ class THaCoincTime+;
#pragma link C++ class THaS2CoincTime+;
#pragma link C++ class THaTrackProj+;
#pragma link C++ class THaPostProcess+;
#pragma link C++ class THaFilter+;
#pragma link C++ class THaElossCorrection+;
#pragma link C++ class THaTrackEloss+;
#pragma link C++ class THaBeamModule+;
#pragma link C++ class THaBeamInfo+;

#ifdef ONLINE_ET
#pragma link C++ class THaOnlRun+;
#endif

#ifdef __MAKECINT__
#pragma link C++ class std::string;
//FIXME: only the templates that we specify here will be available in CINT!!!
#pragma link C++ class std::vector<short>;
#pragma link C++ class std::vector<int>;
#pragma link C++ class std::vector<unsigned short>;
#pragma link C++ class std::vector<unsigned int>;
#pragma link C++ class std::vector<float>;
#pragma link C++ class std::vector<double>;
#pragma link C++ class std::vector<std::string>;
#pragma link C++ class std::vector<std::vector<int>>;
#pragma link C++ class std::vector<std::vector<double>>;
#pragma link C++ class std::vector<TString>;
#pragma link C++ class std::vector<TH1F*>;
#pragma link C++ class std::vector<TH1*>;
#pragma link C++ class std::vector<THaString>;
#pragma link C++ class std::vector<THaVar*>;
#pragma link C++ class std::vector<THaFormula*>;
#pragma link C++ class std::vector<THaCut*>;
#endif

#endif
