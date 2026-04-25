#ifndef Podd_TIClockModule_
#define Podd_TIClockModule_

/////////////////////////////////////////////////////////////////////
//
//   TIClockModule
//
//   Ole Hansen, Apr 2026
//
/////////////////////////////////////////////////////////////////////

#include "GenScaler.h"
#include "Helper.h"

namespace Decoder {

class TIClockModule : public GenScaler {
public:
  TIClockModule() = default;
  TIClockModule( UInt_t crate, UInt_t slot );

  UInt_t LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                   const UInt_t* pstop ) override;
  UInt_t LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                   UInt_t pos, UInt_t len ) override;
  void Init() override;
  void Clear( Option_t* opt="" ) override;

  UInt_t    GetLiveTime()           const { return fData[0]; }
  UInt_t    GetBusyTime()           const { return fData[1]; }
  UInt_t    GetTotalTime()          const { return fData[0] + fData[1]; }
  UInt_t    GetTSScaler( UInt_t i ) const { return i < 6 ? fData[2 + i] : 0; }
  ULong64_t GetEvtNum()             const {
    return (Podd::bitval(fData[9],0, 15) << 16) + fData[10];
  }
  UInt_t    GetBlank5()              const { return fData[11]; }

private:
  static TypeIter_t fgThisType;

  ClassDefOverride(TIClockModule, 0) // TI clock data
};

}

#endif
