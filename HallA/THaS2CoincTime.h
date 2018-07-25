#ifndef Podd_THaS2CoincTime_h_
#define Podd_THaS2CoincTime_h_

//////////////////////////////////////////////////////////////////////////
//
// THaS2CoincTime
//    Calculate coincidence times for all tracks for a pair of
//    spectrometers, USING ONLY S2 for track timing!!!!
//    Assumes a difference in common starts -- timing offsets
//    for different paddles are NOT taken into account here
//    (and shouldn't need to be).
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TString.h"
#include "THaCoincTime.h"
#include "THaVar.h"

class THaS2CoincTime : public THaCoincTime {
  
public:
  THaS2CoincTime( const char* name, const char* description,
		  const char* spec1="L", const char* spec2="R",
		  Double_t mass1 = .938272, Double_t mass2 = 0.000511,
		  const char* ch_name1=0, const char* ch_name2=0,
		  const char* detname1="s2", const char* detname2="");
  
  virtual ~THaS2CoincTime();
  
  virtual Int_t     Process( const THaEvData& );

  virtual EStatus   Init( const TDatime& run_time );
  
protected:
  // store the THaVar variable locations for faster processing
  THaVar* fTrPads1;    //!
  THaVar* fS2TrPath1;  //!
  THaVar* fS2Times1;   //!
  THaVar* fTrPath1;    //!

  THaVar* fTrPads2;    //!
  THaVar* fS2TrPath2;  //!
  THaVar* fS2Times2;   //!
  THaVar* fTrPath2;    //!

  TString fDetName1, fDetName2;
  
public:  

  ClassDef(THaS2CoincTime,0)   // Coinc.Time calc using only S2
};

#endif
