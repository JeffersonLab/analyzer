#ifndef DecoderGlobals_
#define DecoderGlobals_

#include "Rtypes.h"

  static const Int_t MAXROC = 32;  
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

  enum { HED_OK = 0, HED_WARN = -63, HED_ERR = -127, HED_FATAL = -255 };
  enum { MAX_PSFACT = 12 };

#endif
