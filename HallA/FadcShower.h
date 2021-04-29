#ifndef HallA_FadcShower_h_
#define HallA_FadcShower_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// HallA::FadcShower                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaShower.h"

namespace HallA {

class FADCData;

class FadcShower : public THaShower {

public:
  explicit FadcShower( const char* name, const char* description = "",
                       THaApparatus* a = nullptr );
  FadcShower();
  ~FadcShower() override;

protected:
  Int_t     StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;
  OptUInt_t LoadData( const THaEvData& evdata,
                      const DigitizerHitInfo_t& hitinfo ) override;

  Int_t     DefineVariables( EMode mode = kDefine ) override;
  Int_t     ReadDatabase( const TDatime& date ) override;

  FADCData* fFADCData;  // Pointer to FADC readout data (owned by fDetectorData)

  ClassDef(FadcShower, 1)   // Shower detector with FADC readout
};

////////////////////////////////////////////////////////////////////////////////

} // namespace HallA

#endif //HallA_FadcShower_h_
