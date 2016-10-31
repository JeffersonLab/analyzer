#ifndef Decoder_
#define Decoder_

#include "Rtypes.h"

namespace Decoder {
  class Module;               // abstract module
  class VmeModule;            // abstract VME module
  class PipeliningModule;     // piplelining module
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
  class CodaDecoder;
  class Lecroy1875Module;
  class Lecroy1877Module;
  class Lecroy1881Module;
  class Scaler560;
  class Scaler1151;
  class Scaler3800;
  class Scaler3801;
  class Fadc250Module;
  class F1TDCModule;
  class Caen775Module;
  class Caen1190Module;
  class SkeletonModule;

  static const Int_t MAXROC = 32;
  static const Int_t MAXBANK = 4095;
  static const Int_t MAXSLOT = 30;
  static const Int_t MAXSLOT_FB = 26;

  static const Int_t MAX_PHYS_EVTYPE  = 14;  // Types up to this are physics
  static const Int_t SYNC_EVTYPE      = 16;
  static const Int_t PRESTART_EVTYPE  = 17;
  static const Int_t GO_EVTYPE        = 18;
  static const Int_t PAUSE_EVTYPE     = 19;
  static const Int_t END_EVTYPE       = 20;
  static const Int_t TS_PRESCALE_EVTYPE  = 120;
  // should be able to load special event types from crate map
  static const Int_t EPICS_EVTYPE     = 131;
  static const Int_t PRESCALE_EVTYPE  = 133;
  static const Int_t DETMAP_FILE      = 135;
  static const Int_t TRIGGER_FILE     = 136;
  static const Int_t SCALER_EVTYPE    = 140;

 // Access processed data for multi-function modules
  enum EModuleType { kSampleADC, kPulseIntegral, kPulseTime,
		     kPulsePeak, kPulsePedestal, kCoarseTime, kFineTime };


}



#endif
