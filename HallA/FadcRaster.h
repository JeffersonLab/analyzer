#ifndef HallA_FadcRaster_h_
#define HallA_FadcRaster_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaRaster                                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaRaster.h"

namespace HallA {

class FadcRaster : public THaRaster {
public:
  explicit FadcRaster( const char* name, const char* description = "",
                       THaApparatus* a = nullptr );

protected:
  OptUInt_t LoadData( const THaEvData& evdata,
                      const DigitizerHitInfo_t& hitinfo ) override;

  ClassDef(FadcRaster,0)   // Generic Raster class with FADC readouts
};

} //namespace HallA

#endif //HallA_FadcRaster_h_
