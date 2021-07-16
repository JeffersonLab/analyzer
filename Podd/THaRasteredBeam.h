#ifndef Podd_THaRasteredBeam_h_
#define Podd_THaRasteredBeam_h_

////////////////////////////////////////////////////////////////////////////////
//
// THaRasteredBeam
//
// Apparatus describing a rastered beam that is analyzed using event-by-event
// raster currents.
//
////////////////////////////////////////////////////////////////////////////////

#include "THaBeam.h"

class THaRasteredBeam : public THaBeam {

public:
  THaRasteredBeam( const char* name, const char* description,
                   bool do_setup = true );

  virtual Int_t Reconstruct();

protected:

  ClassDef(THaRasteredBeam,0)  // Rastered beam from event-wise raster currents
};

#endif

