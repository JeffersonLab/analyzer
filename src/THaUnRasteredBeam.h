#ifndef ROOT_THaUnRasteredBeam
#define ROOT_THaUnRasteredBeam

//////////////////////////////////////////////////////////////////////////
//
// THaUnRasteredBeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeam.h"

class THaUnRasteredBeam : public THaBeam {

public:
  THaUnRasteredBeam( const char* name, const char* description ) ;

  virtual ~THaUnRasteredBeam() {}
  
  virtual Int_t Reconstruct() ;

protected:


  ClassDef(THaUnRasteredBeam,0)    // A beam with unrastered beam, analyzed event by event
};

#endif

