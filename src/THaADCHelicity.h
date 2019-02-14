#ifndef Podd_THaADCHelicity_h_
#define Podd_THaADCHelicity_h_

////////////////////////////////////////////////////////////////////////
//
// THaADCHelicity
//
// Helicity of the beam - from ADC
// 
////////////////////////////////////////////////////////////////////////


#include "THaHelicityDet.h"

class THaADCHelicity : public THaHelicityDet {

public:

  THaADCHelicity( const char* name, const char* description, 
		  THaApparatus* a = NULL );
  virtual ~THaADCHelicity();

  virtual void   Clear( Option_t* opt = "" );
  virtual Int_t  Decode( const THaEvData& evdata );

  THaADCHelicity()
    : fADC_hdata(0), fADC_Gate(0), fADC_Hel(kUnknown),
      fThreshold(0), fIgnoreGate(kFALSE), fInvertGate(kFALSE),
      fNchan(0) {}  // For ROOT I/O only
  
  // Simplified detector map for the two data channels
  struct ChanDef_t {
    Int_t roc;            // ROC to read out
    Int_t slot;           // Slot of module
    Int_t chan;           // Channel within module
  };

protected:
  // ADC data for helicity and gate
  Double_t   fADC_hdata;  // Helicity ADC raw data
  Double_t   fADC_Gate;   // Gate ADC raw data
  EHelicity  fADC_Hel;    // Calculated beam helicity from ADC data

  Double_t   fThreshold;  // Min ADC amplitude required for Hel = Plus
  Bool_t     fIgnoreGate; // Ignore the gate info and always assign helicity
  Bool_t     fInvertGate; // Invert polarity of gate signal, so that 0=active

  // Simplified detector map for the two data channels
  ChanDef_t  fAddr[2];    // Definitions of helicity and gate channels
  Int_t      fNchan;      // Number of channels to read out (1 or 2)

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaADCHelicity,1)     // Beam helicity from ADC (in time)
};

#endif 

