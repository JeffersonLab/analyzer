#ifndef HallA_FadcRasteredBeam_h_
#define HallA_FadcRasteredBeam_h_

////////////////////////////////////////////////////////////////////////////////
//
// HallA::FadcRasteredBeam
//
// Apparatus describing a rastered beam that is analyzed using event-by-event
// raster currents. Uses the FadcRaster class to read out the raster device.
//
////////////////////////////////////////////////////////////////////////////////

#include "THaRasteredBeam.h"

namespace HallA {

class FadcRasteredBeam : public THaRasteredBeam {
public:
  FadcRasteredBeam( const char* name, const char* description );

  ClassDef(FadcRasteredBeam,0)  // Rastered beam from event-wise raster currents, FADC version
};

} // namespace HallA

#endif // HallA_FadcRasteredBeam_h_
