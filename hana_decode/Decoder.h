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

  inline constexpr UInt_t MAXROC = 32;
  inline constexpr Int_t  MAXBANK = (1<<16)-1;   // bank numbers are uint16_t
  inline constexpr UInt_t MAXSLOT = 32;
  inline constexpr UInt_t MAXSLOT_FB = 26;

  inline constexpr UInt_t MAX_PHYS_EVTYPE  = 14;  // Types up to this are physics
  inline constexpr UInt_t SYNC_EVTYPE      = 16;
  inline constexpr UInt_t PRESTART_EVTYPE  = 17;
  inline constexpr UInt_t GO_EVTYPE        = 18;
  inline constexpr UInt_t PAUSE_EVTYPE     = 19;
  inline constexpr UInt_t END_EVTYPE       = 20;
  inline constexpr UInt_t TS_PRESCALE_EVTYPE  = 120;
  // should be able to load special event types from crate map
  inline constexpr UInt_t EPICS_EVTYPE     = 131; // default in Hall A
  inline constexpr UInt_t PRESCALE_EVTYPE  = 133;
  inline constexpr UInt_t DETMAP_FILE      = 135;
  inline constexpr UInt_t DAQCONFIG_FILE1  = 137;
  inline constexpr UInt_t DAQCONFIG_FILE2  = 138;
  inline constexpr UInt_t TRIGGER_FILE     = 136;
  inline constexpr UInt_t SCALER_EVTYPE    = 140;
  inline constexpr UInt_t SBSSCALER_EVTYPE = 141;
  inline constexpr UInt_t HV_DATA_EVTYPE   = 150;

  inline constexpr UInt_t MAX_PSFACT = 12;
  inline constexpr Int_t  kDefaultPS = 0;

  // Access processed data for multi-function modules
  enum EModuleType { kSampleADC, kPulseIntegral, kPulseTime,
                     kPulsePeak, kPulsePedestal, kCoarseTime, kFineTime };


  enum class ChannelType { kUndefined, kADC, kCommonStopTDC, kCommonStartTDC,
    kMultiFunctionADC, kMultiFunctionTDC };
}



#endif
