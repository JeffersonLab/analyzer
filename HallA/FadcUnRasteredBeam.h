#ifndef HallA_FadcUnRasteredBeam_h_
#define HallA_FadcUnRasteredBeam_h_

////////////////////////////////////////////////////////////////////////////////
//
// HallA::FadcUnRasteredBeam
//
// Apparatus describing a rastered beam that is analyzed using event-by-event
// raster currents. Uses the FadcRaster class to read out the raster device.
//
////////////////////////////////////////////////////////////////////////////////

#include "THaUnRasteredBeam.h"

namespace HallA {

class FadcUnRasteredBeam : public THaUnRasteredBeam {
public:
  FadcUnRasteredBeam( const char* name, const char* description,
                      Int_t runningsum_depth = 0 );

  ClassDef(FadcUnRasteredBeam,0)  // Unrastered beam from two BPMs, FADC version
};

} // namespace HallA

#endif // HallA_FadcUnRasteredBeam_h_
