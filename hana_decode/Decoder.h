#ifndef Podd_Decoder_h_
#define Podd_Decoder_h_

#include "Rtypes.h"

namespace Decoder {
  class Module;               // abstract module
  class VmeModule;            // abstract VME module
  class PipeliningModule;     // pipelining module
  class FastbusModule;        // abstract Fastbus module
  class GenScaler;            // abstract general scaler
  class THaSlotData;          // modified to use modules
  class THaEpics;
  class EpicsChan;
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

  static const UInt_t MAXROC = 32;
  static const Int_t  MAXBANK = (1<<16)-1;   // bank numbers are uint16_t
  static const UInt_t MAXSLOT = 32;
  static const UInt_t MAXSLOT_FB = 26;

  static const UInt_t MAX_PHYS_EVTYPE  = 14;  // Types up to this are physics
  static const UInt_t SYNC_EVTYPE      = 16;
  static const UInt_t PRESTART_EVTYPE  = 17;
  static const UInt_t GO_EVTYPE        = 18;
  static const UInt_t PAUSE_EVTYPE     = 19;
  static const UInt_t END_EVTYPE       = 20;
  static const UInt_t TS_PRESCALE_EVTYPE  = 120;
  // should be able to load special event types from crate map
  static const UInt_t EPICS_EVTYPE     = 131; // default in Hall A
  static const UInt_t PRESCALE_EVTYPE  = 133;
  static const UInt_t DETMAP_FILE      = 135;
  static const UInt_t DAQCONFIG_FILE1  = 137;
  static const UInt_t DAQCONFIG_FILE2  = 138;
  static const UInt_t TRIGGER_FILE     = 136;
  static const UInt_t SCALER_EVTYPE    = 140;
  static const UInt_t SBSSCALER_EVTYPE = 141;
  static const UInt_t HV_DATA_EVTYPE   = 150;

  // Access processed data for multi-function modules
  enum EModuleType { kSampleADC, kPulseIntegral, kPulseTime,
                     kPulsePeak, kPulsePedestal, kCoarseTime, kFineTime };


  enum class ChannelType { kUndefined, kADC, kCommonStopTDC, kCommonStartTDC,
    kMultiFunctionADC, kMultiFunctionTDC };
}



#endif
