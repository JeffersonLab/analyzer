#ifndef Podd_THaRasteredBeam_h_
#define Podd_THaRasteredBeam_h_

//////////////////////////////////////////////////////////////////////////
//
// THaRasteredBeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeam.h"

class THaRasteredBeam : public THaBeam {

public:
  THaRasteredBeam( const char* name, const char* description ) ;

  virtual ~THaRasteredBeam() {}
  
  virtual Int_t Reconstruct() ;

protected:


  ClassDef(THaRasteredBeam,0)    // A beam with rastered beam, analyzed event by event using raster currents
};

#endif

