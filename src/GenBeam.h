#ifndef ROOT_GenBeam
#define ROOT_GenBeam

//////////////////////////////////////////////////////////////////////////
//
// GenBeam
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeam.h"
#include <iostream>
#include <vector>

class GenBeam : public THaBeam {

public:
  GenBeam( const char* name, const char* description, Int_t runningsum_depth = 0 ) ;

  virtual ~GenBeam() {}
  
  virtual Int_t Reconstruct() ;

  void ClearRunningSum();

  Int_t fRunningSumDepth;

protected:

  Bool_t fRunningSumWrap;
  Int_t fRunningSumNext;
  std::vector<TVector3> fRSPosition;
  std::vector<TVector3> fRSDirection;
  TVector3 fRSAvPos;
  TVector3 fRSAvDir;   

  ClassDef(GenBeam,0)    // A beam with unrastered beam, analyzed event by event
};

#endif

