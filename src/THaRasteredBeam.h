#ifndef ROOT_THaRasteredBeam
#define ROOT_THaRasteredBeam

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

