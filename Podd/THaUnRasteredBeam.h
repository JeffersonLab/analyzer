#ifndef Podd_THaUnRasteredBeam_h_
#define Podd_THaUnRasteredBeam_h_

//////////////////////////////////////////////////////////////////////////
//
// THaUnRasteredBeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeam.h"
#include <vector>

class THaUnRasteredBeam : public THaBeam {

public:
  THaUnRasteredBeam( const char* name, const char* description,
                     Int_t runningsum_depth = 0, bool do_setup = true );

  virtual ~THaUnRasteredBeam() = default;
  
  virtual Int_t Reconstruct();

  void ClearRunningSum();

  Int_t fRunningSumDepth;

protected:

  Bool_t fRunningSumWrap;
  Int_t fRunningSumNext;
  std::vector<TVector3> fRSPosition;
  std::vector<TVector3> fRSDirection;
  TVector3 fRSAvPos;
  TVector3 fRSAvDir;   

  ClassDef(THaUnRasteredBeam,0)  // Unrastered beam from two BPMs
};

#endif

