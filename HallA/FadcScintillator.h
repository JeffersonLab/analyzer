#ifndef HallA_FadcScintillator_h_
#define HallA_FadcScintillator_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// HallA::FadcScintillator                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaScintillator.h"
#include "FADCData.h"

namespace HallA {

class FADCData;

class FadcScintillator : public THaScintillator {

public:
  explicit FadcScintillator( const char* name, const char* description = "",
                             THaApparatus* a = nullptr );
  FadcScintillator();
  ~FadcScintillator() override;

protected:
  Int_t    StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;
  OptUInt_t LoadData( const THaEvData& evdata,
                      const DigitizerHitInfo_t& hitinfo ) override;

  Int_t ReadDatabase( const TDatime& date ) override;
  Int_t DefineVariables( EMode mode = kDefine ) override;

  FADCData* fFADCDataL;  // FADC readout data of left-side PMTs
  FADCData* fFADCDataR;  // FADC readout data of right-side PMTs

  ClassDef(FadcScintillator, 1)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

} // namespace HallA

#endif
