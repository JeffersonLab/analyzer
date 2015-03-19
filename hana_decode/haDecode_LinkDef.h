#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class Decoder::CodaDecoder+;
#pragma link C++ class Decoder::Module+;
#pragma link C++ class Decoder::Module::ModuleType+;
#pragma link C++ class Decoder::Module::TypeSet_t+;
#pragma link C++ class Decoder::Module::TypeIter_t+;
#pragma link C++ class Decoder::FastbusModule+;
#pragma link C++ class Decoder::VmeModule+;
#pragma link C++ class Decoder::GenScaler+;
#pragma link C++ class Decoder::Lecroy1877Module+;
#pragma link C++ class Decoder::Lecroy1881Module+;
#pragma link C++ class Decoder::Lecroy1875Module+;
#pragma link C++ class Decoder::Scaler1151+;
#pragma link C++ class Decoder::Scaler3800+;
#pragma link C++ class Decoder::Scaler3801+;
#pragma link C++ class Decoder::Scaler560+;
#pragma link C++ class Decoder::Fadc250Module+;
#pragma link C++ class Decoder::F1TDCModule+;
#pragma link C++ class Decoder::SkeletonModule+;
#pragma link C++ class Decoder::THaCodaData+;
#pragma link C++ class Decoder::THaCodaFile+;
#pragma link C++ class Decoder::THaCrateMap+;
#pragma link C++ class Decoder::THaEpics+;
#pragma link C++ class Decoder::THaFastBusWord+;
#pragma link C++ class Decoder::THaSlotData+;
#pragma link C++ class Decoder::THaUsrstrutils+;
#pragma link C++ class Decoder::THaCodaDecoder+;

#pragma link C++ class THaBenchmark+;
#pragma link C++ class THaEvData+;
#pragma link C++ class THaEvData::RocDat_t+;

#ifndef STANDALONE
#pragma link C++ class Podd::MCHitInfo+;
#pragma link C++ class Podd::MCTrack+;
#pragma link C++ class Podd::MCTrackPoint+;
#pragma link C++ class Podd::SimDecoder+;
#endif

#ifdef ONLINE_ET
#pragma link C++ class Decoder::THaEtClient+;
#endif

#endif
