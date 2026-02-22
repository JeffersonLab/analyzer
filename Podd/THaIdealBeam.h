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

  EStatus Init( const TDatime& run_time ) override;
  Int_t   Reconstruct() override { return 0; }

protected:
  Int_t   ReadRunDatabase( const TDatime& date ) override;

  ClassDefOverride(THaIdealBeam,1)    // A beam with constant position and direction
};

#endif

