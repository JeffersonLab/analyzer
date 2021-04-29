#ifndef HallA_FadcCherenkov_h_
#define HallA_FadcCherenkov_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// HallA::FadcCherenkov                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCherenkov.h"

namespace HallA {

class FADCData;

class FadcCherenkov : public THaCherenkov {

public:
  explicit FadcCherenkov( const char* name, const char* description = "",
                          THaApparatus* a = nullptr );
  FadcCherenkov();
  ~FadcCherenkov() override;

protected:
  Int_t    StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;
  OptUInt_t LoadData( const THaEvData& evdata,
                      const DigitizerHitInfo_t& hitinfo ) override;

  Int_t    DefineVariables( EMode mode = kDefine ) override;
  Int_t    ReadDatabase( const TDatime& date ) override;

  FADCData* fFADCData;  // Pointer to FADC readout data (owned by fDetectorData)

  ClassDef(FadcCherenkov, 1)   // Cherenkov detector with FADC readout
};

////////////////////////////////////////////////////////////////////////////////

} // namespace HallA

#endif //HallA_FadcCherenkov_h_
