////////////////////////////////////////////////////////////////////////
//
// THaADCHelicity
//
// Helicity of the beam - from ADC
// 
//
// Debug levels for this detector: (set via SetDebug(n))
// = 1 for messages about unusual helicity bit handling,
//     e.g. handling missing quadruples
// = 2 for messages relating to scalers
// = 3 for verbose debugging output
// = -1 to get fTdiff (calib time)
//
////////////////////////////////////////////////////////////////////////


#ifndef ROOT_THaADCHelicity
#define ROOT_THaADCHelicity

#include "THaHelicityDet.h"

class THaADCHelicity : public THaHelicityDet {

public:

  THaADCHelicity( const char* name, const char* description, 
		  THaApparatus* a = NULL );
  virtual ~THaADCHelicity();

  virtual void   Clear( Option_t* opt = "" );
  virtual Int_t  Decode( const THaEvData& evdata );


  // The user may set the state of this class (see implementation)
//   void SetState(Int_t mode, Int_t delay, Int_t sign, 
// 		Int_t spec, Int_t redund);

  THaADCHelicity();  // For ROOT I/O only

protected:

// ADC data for helicity and gate (redundant).
  Double_t   fADC_hdata; // Helicity ADC raw data
  Double_t   fADC_Gate;  // Gate ADC raw data
  EHelicity  fADC_Hel;   // Calculated beam helicity from ADC data

  Double_t   fThreshold;  // Min ADC amplitude required for Hel = Plus
  Bool_t     fIgnoreGate; // Ignore the gate info and always assign helicity

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaADCHelicity,1)     // Beam helicity from ADC (in time)
};

#endif 

