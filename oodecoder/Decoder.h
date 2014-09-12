#ifndef Decoder_
#define Decoder_

namespace Decoder {
  class THaEvData;            // abstract decoder
  class Module;               // abstract module   (new)
  class VmeModule;            // abstract VME module  (new)
  class FastbusModule;        // abstract Fastbus module  (new)
  class GenScaler;            // abstract general scaler  (new)
  class THaCodaDecoder;       // older decoder, not touched 
  class THaSlotData;          // modified to use modules
  class THaCrateMap;          // used and unmodified
  class THaUsrstrutils;       // used and unmodified
  class THaCodaData;          // used and unmodified
  class THaCodaFile;          // used and unmodified
  class THaEtClient;          // used and unmodified
  class CodaDecoder;          // OO decoder; this and the rest are all new
  class Lecroy1875Module;     // the rest are specific modules
  class Lecroy1877Module;
  class Lecroy1881Module;
  class Scaler560;
  class Scaler1151;
  class Scaler3800;
  class Scaler3801;
  class Fadc250Module;
  // will need a few more module classes like 1191, F1TDC, etc.
}

#endif
