#ifndef ROOT_THaBeam
#define ROOT_THaBeam

//////////////////////////////////////////////////////////////////////////
//
// THaBeam
//
// The Hall A BPM and Raster data class.
// 
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"

class THaBeam : public THaApparatus {
  
public:
  THaBeam( const char* description="" );
  virtual ~THaBeam() {}
  
  virtual Int_t  Reconstruct();

  ClassDef(THaBeam,0)  
};

#endif

