#ifndef ROOT_THaIdealBeam
#define ROOT_THaIdealBeam

//////////////////////////////////////////////////////////////////////////
//
// THaIdealBeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeam.h"

class THaIdealBeam : public THaBeam {

public:
  THaIdealBeam( const char* name, const char* description ) :
    THaBeam( name, description ) {}

  virtual ~THaIdealBeam() {}
  
  virtual Int_t Reconstruct() { return 0; }

protected:

  virtual Int_t ReadRunDatabase( FILE* file, const TDatime& date );

  ClassDef(THaIdealBeam,1)    // A beam with constant position and direction
};

#endif

