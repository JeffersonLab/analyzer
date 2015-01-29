#ifndef Decoder_
#define Decoder_

namespace Decoder {
  class Module;               // abstract module  
  class VmeModule;            // abstract VME module  
  class FastbusModule;        // abstract Fastbus module  
  class GenScaler;            // abstract general scaler  
  class THaCodaDecoder;       // older decoder, obsolescent 
  class THaSlotData;          // modified to use modules
  class THaEpics;
  class EpicsChan;
  class THaFastBusWord;
  class THaCrateMap;          
  class THaUsrstrutils;       
  class THaCodaData;          
  class THaCodaFile;          
  class THaEtClient;          
  class CodaDecoder;          // OO decoder; this and the following are new
  class Lecroy1875Module;     
  class Lecroy1877Module;
  class Lecroy1881Module;
  class Scaler560;
  class Scaler1151;
  class Scaler3800;
  class Scaler3801;
  class Fadc250Module;
}

#endif
