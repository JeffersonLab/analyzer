#ifndef HallA_FadcBPM_h_
#define HallA_FadcBPM_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// HallA::FadcBPM                                                            //
//                                                                           //
// Class for handling BPMs with FADC250 frontends                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaBPM.h"

namespace HallA {

class FadcBPM : public THaBPM {

public:
  explicit FadcBPM( const char* name, const char* description = "",
                    THaApparatus* a = nullptr );
  FadcBPM() = default;

protected:
  OptUInt_t LoadData( const THaEvData& evdata,
                      const DigitizerHitInfo_t& hitinfo ) override;

  ClassDef(FadcBPM,0)   // Generic BPM class
};

////////////////////////////////////////////////////////////////////////////////

} // namespace HallA

#endif //HallA_FadcBPM_h_
