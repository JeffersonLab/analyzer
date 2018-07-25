#ifndef Podd_THaHelicity_h_
#define Podd_THaHelicity_h_

//////////////////////////////////////////////////////////////////////////
//
// THaHelicity
//
// In-time helicity detector - both from ADC and G0 electronics 
// (with delay=0).
// Provides redundancy for cross-checks.
//
//////////////////////////////////////////////////////////////////////////


#include "THaADCHelicity.h"
#include "THaG0HelicityReader.h"

class THaHelicity : public THaADCHelicity, public THaG0HelicityReader {
  
public:
  THaHelicity( const char* name, const char* description, 
	       THaApparatus* a = NULL );
  virtual ~THaHelicity();

  virtual void   Clear( Option_t* opt = "" );
  virtual Int_t  Decode( const THaEvData& evdata );

  THaHelicity();  // For ROOT RTTI
  
protected:

  // Event-by-event data
  Int_t  fG0_Hel;           // G0 helicity reading
  Bool_t fGoodHel;          // ADC and G0 helicities agree
  Bool_t fGoodHel2;         // ADC and G0 helicities agree unless one unknown

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(THaHelicity,2)   // In-time helicity from ADC and G0 readout

};

#endif

