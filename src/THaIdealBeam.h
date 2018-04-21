#ifndef Podd_THaIdealBeam_h_
#define Podd_THaIdealBeam_h_

//////////////////////////////////////////////////////////////////////////
//
// THaIdealBeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeam.h"

class THaIdealBeam : public THaBeam {

public:
  THaIdealBeam( const char* name, const char* description );

  virtual ~THaIdealBeam() {}
  
  virtual EStatus Init( const TDatime& run_time );
  virtual Int_t   Reconstruct() { return 0; }

protected:

  virtual Int_t   ReadRunDatabase( const TDatime& date );

  ClassDef(THaIdealBeam,1)    // A beam with constant position and direction
};

#endif

